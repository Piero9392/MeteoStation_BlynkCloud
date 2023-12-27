// Include the LoRa SPI-connection library.
#include <SPI.h>
#include <LoRa.h>

// Include the SSD1306 OLED library.
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Include the WiFi library for ESP32.
#include <WiFi.h>

// Include the WiFiClient library for creating a client that connects to a specified internet IP address and port.
#include <WiFiClient.h>

// Include the Time NTP Server library.
#include "time.h"

// Include the AceButton library
#include <AceButton.h>

// Define the GPIOs SPI-connection with LoRa.
#define SS 5
#define RST 14
#define DI0 2

// LED pins.
#define rssiLed 4
#define dataLed 13
#define wifiLed 12

// Physical button pin.
#define buttonPin 17

// Virtual button pin.
#define VPIN_Button V8

/* Set your Blynk ID and Blynk NAME from your Blynk.Cloud account.
WARNING!
In this case "#define BLYNK_TEMPLATE_ID" and "#define BLYNK_TEMPLATE_NAME" statements must stay before the library "#include <BlynkSimpleEsp32.h>" directive. */
#define BLYNK_TEMPLATE_ID "TMPL4Px-ujJX3"
#define BLYNK_TEMPLATE_NAME "WeatherStation"
// Include the Blynk library for ESP32.
#include <BlynkSimpleEsp32.h>

// Variable to indicate the status of the Wi-Fi connection.
int wifiFlag = 0;

// Set your WiFi Network and WiFi password.
const char* ssid = "TP-Link_1FAA";
const char* password = "10458327";

// Set your Blynk auth token from your Blynk.Cloud account.
char auth[] = "p_rzoVEw6nYdmsumm99w3a3pVG8CRe_t";

// Flag indicating whether to fetch Blynk state during initialization.
bool fetch_blynk_state = true;

/* Time NTP Server.
gmtOffset_sec - the offset in seconds between your time zone and GMT.
daylightOffset_sec - the offset in seconds for your daylight saving time. */
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

// Define integer to remember physical switch-button toggle state.
bool toggleState = 0;

// Define integer to remember OLED state mode.
int displayMode = 0;

// Variables for LoRa telemetry.
String device_id;
String temperature;
String humidity;
String pressure;
String gas;
String dewPoint;
String brightness;
String altitude;

// Set OLED definition.
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// BlynkTimer is a timer utility provided by the Blynk library for scheduling tasks and periodic operations.
BlynkTimer timer;

// AceButton configuration and instance.
using namespace ace_button;
ButtonConfig config;
AceButton button(&config);

// Function prototype for the button event handler.
void handleEvent(AceButton*, uint8_t, uint8_t);

// Setup function - runs once when the microcontroller starts or is reset.
void setup() {
  // Initialize serial communication with a baud rate of 115200.
  Serial.begin(115200);

  // Set the mode for the physical button pin to INPUT_PULLUP. INPUT_PULLUP configures the pin as an input with an internal pull-up resistor.
  pinMode(buttonPin, INPUT_PULLUP);

  // Set the mode for the LED pins to OUTPUT.
  pinMode(rssiLed, OUTPUT);
  pinMode(dataLed, OUTPUT);
  pinMode(wifiLed, OUTPUT);

  // Turn off the WiFi LED initially by setting it to LOW.
  digitalWrite(wifiLed, LOW);

  // Initialize OLED display.
  startOled();
  
  // Initialize WiFi connection.
  startWiFi();

  // Initialize LoRa module.
  startLora();
  
  // Initialize the Blynk connection with the specified authentication token, WiFi SSID, and password.
  Blynk.begin(auth, ssid, password);

  // Configure Blynk with the provided authentication token.
  Blynk.config(auth);

  // Check if Blynk.Cloud server is connected every 5000 milliseconds.
  timer.setInterval(5000L, checkBlynkStatus);
  
  // Configuration for your GMT time zone and daylight saving time (NTP Server).
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // During starting OLED should have Time/Date Mode.
  displayMode = !toggleState;
  oledMode();
  
  // Set the event handler for the physical button using the AceButton library.
  config.setEventHandler(buttonHandler);

  // Initialize the virtual button with the specified physical button pin.
  button.init(buttonPin);

  // Delay to ensure Blynk configuration is completed before further execution.
  delay(500);

  // If fetch_blynk_state is false, synchronize the virtual button state with the toggleState.
  if (!fetch_blynk_state) {
    Blynk.virtualWrite(VPIN_Button, toggleState);
  }
}

// Main loop function where the program execution continuously loops.
void loop() {
  // Read LoRa data, send it to Blynk, and update local variables.
  dataLora();

  // Process Blynk tasks and handle communication with Blynk server.
  Blynk.run();

  // Run tasks scheduled by SimpleTimer.
  timer.run();

  // Check the state of the physical button and update OLED display mode.
  button.check();

  // Update the OLED display mode based on the current state.
  oledMode();

  // Check the conditions and perform tasks accordingly - low RSSI signal.
  taskCondition();
}

// Function to initialize LoRa module.
void startLora() {
  // Set the SPI pins for LoRa communication.
  LoRa.setPins(SS, RST, DI0);

  // Attempt to start the LoRa module at a specific frequency (433 MHz in this case).
  if (!LoRa.begin(433E6)) {
    // If starting LoRa fails, print an error message and turn on the RSSI LED indicator.
    Serial.println("Starting LoRa failed");
    digitalWrite(rssiLed, HIGH);

    // Enter an infinite loop, halting further program execution.
    while (1);
  }
}

// Function to initialize Wi-Fi network connection.
void startWiFi() {
  // Print a message indicating the start of the connection process.
  Serial.print("Connecting to ");
  // Print the SSID to which the connection is being attempted.
  Serial.print(ssid);

  // Begin the WiFi connection with the specified SSID and password.
  WiFi.begin(ssid, password);

  // Variable to count the number of connection attempts.
  int attempts = 0;

  // Wait for the WiFi connection to be established.
  while (WiFi.status() != WL_CONNECTED) {
    // Delay for 100 milliseconds.
    delay(100);
    // Print a dot to indicate progress.
    Serial.print(".");
    // Increment the attempts counter.
    attempts++;

    // Limit the number of attempts to connect to WiFi.
    if (attempts > 50) {
      // If the maximum attempts are reached, print a failure message to the Serial monitor, to the OLED screen and exit the loop.
      wifiFailure();
      break;
    }
  }

  // If WiFi is connected, print relevant information to the Serial monitor.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
}

// Function to handle WiFi failure.
void wifiFailure() {
  // Print a message to the Serial monitor indicating that the WiFi connection failed.
  Serial.println("\nWiFi failed!");

  // Print the WiFi SSID and password for debugging purposes to the Serial monitor.
  Serial.print("WiFi: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  //Clear the display to prepare for new data.
  display.clearDisplay();
  
  // Set text color to white.
  display.setTextColor(WHITE);
  
  // Set text size to 1.
  display.setTextSize(1);
  
  // Set the starting cursor position on the OLED screen.
  display.setCursor(30, 0);
  
  // Display "WiFi failed! on the OLED screen.
  display.print("WiFi failed!");

  // Set the starting cursor position on the OLED screen.
  display.setCursor(15, 10);
  
  // Display "Check the network" on the OLED screen.
  display.print("Check the network");
  
  // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 18);

  // Display "---------------------" on the OLED screen.
  display.print("---------------------");

    // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 27);

  // Display "WiFi" on the OLED screen.
  display.print("WiFi");
  
  // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 37);

  // Display WiFi network name (ssid) on the OLED screen.
  display.print(ssid);

  // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 47);

  // Display "Password" on the OLED screen.
  display.print("Password");

   // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 57);

  // Display WiFi network password on the OLED screen.
  display.print(password);

  // Update the OLED display.
  display.display();
}

// Function to initialize the OLED SSD1306 display.
void startOled() {
  // Begin communication with the OLED display using the SSD1306_SWITCHCAPVCC mode and I2C address 0x3C.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Clear the OLED display buffer.
  display.clearDisplay();

  // Display the cleared buffer on the OLED screen.
  display.display();
}

// Function to read data from LoRa, process it, and send it to the Blynk.Cloud server.
void dataLora() {
  // Variables to store positions in the LoRaData string.
  int pos1, pos2, pos3, pos4, pos5, pos6, pos7;

  // Check if there is a valid LoRa packet.
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read the LoRa data into a string.
    String LoRaData = LoRa.readString();
    while (LoRa.available()) {
      // Print the received LoRa data to the Serial Monitor.
      Serial.print((char)LoRa.read());
    }

    // Extract data from the LoRaData string using delimiters.
    pos1 = LoRaData.indexOf('/');
    pos2 = LoRaData.indexOf('&');
    pos3 = LoRaData.indexOf('#');
    pos4 = LoRaData.indexOf('@');
    pos5 = LoRaData.indexOf('$');
    pos6 = LoRaData.indexOf('^');
    pos7 = LoRaData.indexOf('!');
    
    // Define variables from the extracted substrings LoRa.readString().
    device_id = LoRaData.substring(0, pos1);
    temperature = LoRaData.substring(pos1 + 1, pos2);
    humidity = LoRaData.substring(pos2 + 1, pos3);
    pressure = LoRaData.substring(pos3 + 1, pos4);
    gas = LoRaData.substring(pos4 + 1, pos5);
    dewPoint = LoRaData.substring(pos5 + 1, pos6);
    brightness = LoRaData.substring(pos6 + 1, pos7);
    altitude = LoRaData.substring(pos7 + 1, LoRaData.length());
    
    // Flash the yellow LED when LoRa data is received.
    digitalWrite(dataLed, HIGH);
    delay(50);
    digitalWrite(dataLed, LOW);
    
    // Get time and date information from NTP Server.
    timeServer();

    // Print LoRa telemetry to the Serial monitor.
    dataSerial();

    // Send data to the Blynk.Cloud server.
    dataBlynk();
  }
}

// Function to send LoRa telemetry data to the Blynk.Cloud server.
void dataBlynk() {
  // Send temperature to Blynk virtual pin V0.
  Blynk.virtualWrite(V0, temperature);
  
  // Send humidity to Blynk virtual pin V1.
  Blynk.virtualWrite(V1, humidity);
  
  // Send pressure to Blynk virtual pin V2.
  Blynk.virtualWrite(V2, pressure);
  
  // Send gas to Blynk virtual pin V3.
  Blynk.virtualWrite(V3, gas);
  
  // Send dew point to Blynk virtual pin V4.
  Blynk.virtualWrite(V4, dewPoint);
  
  // Send brightness to Blynk virtual pin V5.
  Blynk.virtualWrite(V5, brightness);
  
  // Send altitude to Blynk virtual pin V6.
  Blynk.virtualWrite(V6, altitude);
  
  // Send LoRa packet RSSI to Blynk virtual pin V7.
  Blynk.virtualWrite(V7, LoRa.packetRssi());
}

// Function to set the OLED display mode based on the current displayMode variable.
void oledMode() {
  // Use a switch statement to determine the display mode.
  switch (displayMode) {
    // If displayMode is "0", display Time/Date information from the NTP server on the OLED screen.
    case 0:
      timeOled();
      break;
    // If displayMode is "1", display Telemetry data from the LoRa on the OLED screen.
    case 1:
      dataOled();
      break;
  }
}

// Function to display telemetry data on the OLED screen.
void dataOled() {
  // Clear the display to prepare for new data.
  display.clearDisplay();
  
  // Set text color to white.
  display.setTextColor(WHITE);
  
  // Set text size to 1.
  display.setTextSize(1);
  
  // Set the starting cursor position on the OLED screen.
  display.setCursor(0, 0);
  
  // Display temperature data.
  display.print("TEMPERATURE| ");
  display.print(temperature);
  display.println(" C");
  
  // Display humidity data.
  display.print("HUMIDITY   | ");
  display.print(humidity);
  display.println(" %");
  
  // Display pressure data.
  display.print("PRESSURE   | ");
  display.print(pressure);
  display.println(" hPa");
  
  // Display gas data.
  display.print("GAS        | ");
  display.print(gas);
  display.println(" kOhm");
  
  // Display dew point data.
  display.print("DEW POINT  | ");
  display.print(dewPoint);
  display.println(" C");
  
  // Display brightness data.
  display.print("BRIGHTNESS | ");
  display.print(brightness);
  display.println(" lx");
  
  // Display altitude data.
  display.print("ALTITUDE   | ");
  display.print(altitude);
  display.println(" m");
  
  // Display LoRa RSSI level (dB).
  display.print("RSSI       | ");
  display.print(LoRa.packetRssi());
  display.println(" dB");
  
  // Update the OLED display.
  display.display();
}

// Function to display Time/Date on the OLED screen from NTP Server.
void timeOled() {
  // Structure to store time and date information.
  struct tm timeinfo;
  
  // Retrieve local time and date information.
  getLocalTime(&timeinfo);

  // Clear the OLED display.
  display.clearDisplay();

  // Set text color to white.
  display.setTextColor(WHITE);

  // Draw a rounded rectangle for visual aesthetics.
  display.drawRoundRect(9, 13, 111, 26, 5, WHITE);

  // Set text size to 1.
  display.setTextSize(1);

  // Set cursor position for the city name.
  display.setCursor(54, 0);
  display.print("KYIV ");

  // Set cursor position for the day of the week.
  display.setCursor(0, 45);
  display.println(&timeinfo, "%A");

  // Set cursor position for the day, month, and year.
  display.setCursor(0, 57);
  display.println(&timeinfo, "%d %B %Y");

  // Set text size to 2 for the time.
  display.setTextSize(2);

  // Set cursor position for the time.
  display.setCursor(18, 19);
  display.println(&timeinfo, "%H:%M:%S");

  // Update the OLED display.
  display.display();
}

// Function to print LoRa telemetry to the Serial monitor.
void dataSerial() {
  // Print a header indicating the reception of telemetry data from LoRa.
  Serial.println(F("Received TELEMETRY from LoRa:"));

  // Print the device ID.
  Serial.print(F("Device ID   | "));
  Serial.println(device_id);

  // Print the received signal strength indicator (RSSI) in decibels.
  Serial.print(F("RSSI        | "));
  Serial.print(LoRa.packetRssi());
  Serial.println(F(" dB"));

  // Print temperature data.
  Serial.print(F("Temperature | "));
  Serial.print(temperature);
  Serial.println(F(" °C"));

  // Print humidity data.
  Serial.print(F("Humidity    | "));
  Serial.print(humidity);
  Serial.println(F(" %"));

  // Print atmospheric pressure data.
  Serial.print(F("Pressure    | "));
  Serial.print(pressure);
  Serial.println(F(" hPa"));

  // Print gas resistance data.
  Serial.print(F("Gas         | "));
  Serial.print(gas);
  Serial.println(F(" kOhm"));

  // Print dew point data.
  Serial.print(F("Dew Point   | "));
  Serial.print(dewPoint);
  Serial.println(F(" °C"));

  // Print brightness data.
  Serial.print(F("Brightness  | "));
  Serial.print(brightness);
  Serial.println(F(" Lx"));

  // Print altitude data.
  Serial.print(F("Altitude    | "));
  Serial.print(altitude);
  Serial.println(F(" m"));

  // Print an empty line for better readability.
  Serial.println();
}

// Function to get Time/Date from NTP Server https://www.ntppool.org, save them to the 'timeinfo' structure. Print Time/Date to the Serial monitor.
void timeServer() {
  // Structure to store time and date information.
  struct tm timeinfo;

  // Check if the local time and date information can be obtained from the NTP server.
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Time/Date failed!");
    return;
  } else {
    // Print the received time and date information to the Serial monitor.
    Serial.println(F("Received TIME/DATE from www.ntppool.org:"));
    Serial.print("KYIV ");
    Serial.println(&timeinfo, "%H:%M:%S/%A/%B %d/%Y");
    Serial.println();
  }
}

// Function to check the connection status with the Blynk.Cloud server.
void checkBlynkStatus() {
  // Check if the device is connected to the Blynk.Cloud server.
  bool isconnected = Blynk.connected();

  // If not connected to Blynk.Cloud.
  if (isconnected == false) {
    // Set the Wi-Fi flag to indicate disconnection.
    wifiFlag = 1;

    // Print a message indicating that Blynk is not connected.
    Serial.println("Blynk Not Connected");

    // Turn off the Wi-Fi LED.
    digitalWrite(wifiLed, LOW);
  }

  // If connected to Blynk.Cloud server.
  if (isconnected == true) {
    // Reset the Wi-Fi flag to indicate connection.
    wifiFlag = 0;

    // If the fetch_blynk_state is false, update the virtual button state on Blynk.Cloud app.
    if (!fetch_blynk_state){
      Blynk.virtualWrite(VPIN_Button, toggleState);
    }

    // Turn on the Wi-Fi LED.
    digitalWrite(wifiLed, HIGH);

    // Print a message indicating that Blynk is connected.
    Serial.println("Blynk Connected");
    Serial.println();
  }
}

// Blynk connected callback function.
BLYNK_CONNECTED() {
  // Check if fetch_blynk_state is true.
  if (fetch_blynk_state) {
    // Synchronize the state of the virtual button with Blynk.
    Blynk.syncVirtual(VPIN_Button);
  }
}

// Blynk callback function for handling changes in the virtual button state.
BLYNK_WRITE(VPIN_Button) {
  // Get the new state of the virtual button (0 or 1).
  toggleState = param.asInt();

  // Toggle the display mode based on the virtual button state.
  displayMode = !toggleState;

  // Update the OLED display based on the new display mode.
  oledMode();
}

// Function to handle events from the physical button using AceButton library.
void buttonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Check the type of event.
  switch (eventType) {
    // Event triggered when the physical button is released.
    case AceButton::kEventReleased:
      // Toggle the display mode based on the current state of the physical switch.
      displayMode = toggleState;
      // Update the OLED display based on the new display mode.
      oledMode();
      // Toggle the state of the physical switch.
      toggleState = !toggleState;
      // Send the updated state to the Blynk.Cloud server.
      Blynk.virtualWrite(VPIN_Button, toggleState);
      break;
  }
}

// Function to check the condition of the LoRa signal strength and control the red LED indicator accordinaly of signal (if signal is less than -120dB flash the red Led).
void taskCondition() {
  // Check if the received signal strength indicator (RSSI) is below or equal to -120 dB.
  if (LoRa.packetRssi() <= -120) {
    // If the RSSI is below the threshold, turn on the LED indicator to indicate a low signal strength.
    digitalWrite(rssiLed, HIGH);
  } else {
    // If the RSSI is above the threshold, turn off the LED indicator.
    digitalWrite(rssiLed, LOW);
  }
}
