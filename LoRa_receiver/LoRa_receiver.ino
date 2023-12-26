// Define the GPIO SPI connection with LoRa
#include <SPI.h>
#include <LoRa.h>                              
#define SS 5
#define RST 14
#define DI0 2

// Variables for LoRa telemetry
String device_id;
String temperature;
String humidity;
String pressure;
String gas;
String dewPoint;
String brightness;
String altitude;

/* Time NTP Server library
gmtOffset_sec - the offset in seconds between your time zone and GMT daylightOffset_sec - the offset in seconds for your daylight saving time */
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

// Set your Blynk ID and Blynk NAME from your Blynk.Cloud account
#define BLYNK_TEMPLATE_ID "TMPL4Px-ujJX3"
#define BLYNK_TEMPLATE_NAME "WeatherStation"
// Blynk auth token from your Blynk.Cloud account
char auth[] = "p_rzoVEw6nYdmsumm99w3a3pVG8CRe_t";
bool fetch_blynk_state = true;

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <AceButton.h>
using namespace ace_button;
int wifiFlag = 0;

// Set your WiFi Network and WiFi password
const char* ssid = "TP-Link_1FAA";
const char* password = "10458327";

// SSD1306 OLED library and oled definition
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>                     
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// LED pins
#define rssiLed 4
#define dataLed 13
#define wifiLed 12

// Virtual button pin
#define VPIN_Button V8

// Physical button pin
#define buttonPin 17

// Define integer to remember the toggle state for button
bool toggleState = 0;

// Define integer to remember OLED state mode
int displayMode = 0;

ButtonConfig config;
AceButton button(&config);
void handleEvent(AceButton*, uint8_t, uint8_t);

BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(rssiLed, OUTPUT);
  pinMode(dataLed, OUTPUT);
  
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);
  
  // Initialize WiFi connection
  startWiFi();
  
  Blynk.begin(auth, ssid, password);
  
  // Initialize LoRa module
  startLora();
  
  // Initialize OLED display
  startOled();
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // During starting OLED should have Time/Date Mode
  displayMode = !toggleState;
  oledMode();
  
  config.setEventHandler(buttonHandler);
  button.init(buttonPin);
  
  //Check if Blynk.Cloud server is connected every 5 seconds
  timer.setInterval(5000L, checkBlynkStatus);
  Blynk.config(auth);
  delay(500);
  if (!fetch_blynk_state) {
    Blynk.virtualWrite(VPIN_Button, toggleState);
  }
}

void loop() {
  dataLora();
  Blynk.run();
  // Initiates SimpleTimer
  timer.run();
  button.check();
  oledMode();
  taskCondition();
}

// Initialize LoRa module
void startLora() {
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed");
    digitalWrite(rssiLed, HIGH);
    while (1);
  }
}

// Initialize Wi-Fi network
void startWiFi() {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    attempts++;
    // Time of attempts to connect to WiFi in seconds
    if (attempts > 50) {
      Serial.println("\nWiFi failed");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
}

// Initialize OLED SSD1306
void startOled() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

// Read data from loRa, send it to the Blynk.Cloud
void dataLora() {
  // Variables for LoRa packet
  int pos1, pos2, pos3, pos4, pos5, pos6, pos7;
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String LoRaData = LoRa.readString();
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    pos1 = LoRaData.indexOf('/');
    pos2 = LoRaData.indexOf('&');
    pos3 = LoRaData.indexOf('#');
    pos4 = LoRaData.indexOf('@');
    pos5 = LoRaData.indexOf('$');
    pos6 = LoRaData.indexOf('^');
    pos7 = LoRaData.indexOf('!');
    
    // Define variables from LoRa.readString()
    device_id = LoRaData.substring(0, pos1);
    temperature = LoRaData.substring(pos1 + 1, pos2);
    humidity = LoRaData.substring(pos2 + 1, pos3);
    pressure = LoRaData.substring(pos3 + 1, pos4);
    gas = LoRaData.substring(pos4 + 1, pos5);
    dewPoint = LoRaData.substring(pos5 + 1, pos6);
    brightness = LoRaData.substring(pos6 + 1, pos7);
    altitude = LoRaData.substring(pos7 + 1, LoRaData.length());
    
    // Flash Yellow LED when LoRa data received
    digitalWrite(dataLed, HIGH);
    delay(50);
    digitalWrite(dataLed, LOW);
    
    // Get Date/Time from NTP Server
    timeServer();
    
    // Serial print LoRa telemetry
    Serial.println(F("Received TELEMETRY from LoRa:"));
    Serial.print(F("Device ID   | "));
    Serial.println(device_id);
    Serial.print(F("RSSI        | "));
    Serial.print(LoRa.packetRssi());
    Serial.println(F(" dB"));
    Serial.print(F("Temperature | "));
    Serial.print(temperature);
    Serial.println(F(" °C"));
    Serial.print(F("Humidity    | "));
    Serial.print(humidity);
    Serial.println(F(" %"));
    Serial.print(F("Pressure    | "));
    Serial.print(pressure);
    Serial.println(F(" hPa"));
    Serial.print(F("Gas         | "));
    Serial.print(gas);
    Serial.println(F(" kOhm"));
    Serial.print(F("Dew Point   | "));
    Serial.print(dewPoint);
    Serial.println(F(" °C"));
    Serial.print(F("Brightness  | "));
    Serial.print(brightness);
    Serial.println(F(" Lx"));
    Serial.print(F("Altitude    | "));
    Serial.print(altitude);
    Serial.println(F(" m"));
    Serial.println();

    // Send data to the Blynk.Cloud
    dataBlynk();
  }
}

// Get Time/Date from NTP Server https://www.ntppool.org. Print to the Serial Port
void timeServer() {
  // Get Time/Date from https://www.ntppool.org, save them to the 'timeinfo' structure.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("TIME failed!");
    return;
  } else {
    Serial.println(F("Received TIME/DATE from www.ntppool.org:"));
    Serial.print("KYIV ");
    Serial.println(&timeinfo, "%H:%M:%S/%A/%B %d/%Y");
    Serial.println();

  }
}

// When the Application Blynk IoT button is pushed - switch the state
BLYNK_WRITE(VPIN_Button) {
  toggleState = param.asInt();
  displayMode = !toggleState;
  oledMode();
}

// Check Blynk.Cloud server connection
void checkBlynkStatus() {
  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    wifiFlag = 1;
    Serial.println("Blynk Not Connected");
    digitalWrite(wifiLed, LOW);
  }
  if (isconnected == true) {
    wifiFlag = 0;
    if (!fetch_blynk_state){
      Blynk.virtualWrite(VPIN_Button, toggleState);
    }
    digitalWrite(wifiLed, HIGH);
    Serial.println("Blynk Connected");
    Serial.println();
  }
}

BLYNK_CONNECTED() {
  if (fetch_blynk_state) {
    Blynk.syncVirtual(VPIN_Button);
  }
}

// Send LoRa data to the Blynk.Cloud
void dataBlynk() {
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, pressure);
  Blynk.virtualWrite(V3, gas);
  Blynk.virtualWrite(V4, dewPoint);
  Blynk.virtualWrite(V5, brightness);
  Blynk.virtualWrite(V6, altitude);
  Blynk.virtualWrite(V7, LoRa.packetRssi());
}

void buttonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      displayMode = toggleState;
      oledMode();
      toggleState = !toggleState;
      Blynk.virtualWrite(VPIN_Button, toggleState);
      break;
  }
}

// Display data on the OLED by switch-button
void oledMode() {
  switch (displayMode) {
    case 0:
      dataOled();
      break;
    case 1:
      timeOled();
      break;
  }
}

// Display telemetry on the OLED
void dataOled() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("TEMPERATURE| ");
  display.print(temperature);
  display.println(" C");
  display.print("HUMIDITY   | ");
  display.print(humidity);
  display.println(" %");
  display.print("PRESSURE   | ");
  display.print(pressure);
  display.println(" hPa");
  display.print("GAS        | ");
  display.print(gas);
  display.println(" kOhm");
  display.print("DEW POINT  | ");
  display.print(dewPoint);
  display.println(" C");
  display.print("BRIGHTNESS | ");
  display.print(brightness);
  display.println(" lx");
  display.print("ALTITUDE   | ");
  display.print(altitude);
  display.println(" m");
  display.print("RSSI       | ");
  // LoRa RSSI level (dB)
  display.print(LoRa.packetRssi());
  display.println(" dB");
  display.display();
}

// Display Time/Date on the OLED from NTP Server https://www.ntppool.org
void timeOled() {
  // Get Time/Date from https://www.ntppool.org, save them to the 'timeinfo' structure.
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.drawRoundRect(9, 13, 111, 26, 5, WHITE);
  display.setTextSize(1);
  display.setCursor(54, 0);
  display.print("KYIV ");
  display.setCursor(0, 45);
  display.println(&timeinfo, "%A");
  display.setCursor(0, 57);
  display.println(&timeinfo, "%d %B %Y");
  display.setTextSize(2);
  display.setCursor(18, 19);
  display.println(&timeinfo, "%H:%M:%S");
  display.display();
}

// Low RSSI-level indication by the Red LED (if less than -120dB)
void taskCondition() {
  if (LoRa.packetRssi() <= -120) {
    digitalWrite(rssiLed, HIGH);
  } else {
    digitalWrite(rssiLed, LOW);
  }
}
