// Libraries for I2C interface wiring (BME680, BH1750) and SPI interface wiring (LoRa).
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <BH1750.h>
#define SEALEVELPRESSURE_HPA (1013.25)

// Variable for data sending interval (ms)
#define transmitInterval 5000

Adafruit_BME680 bme;
BH1750 lightMeter;
String LoRaMessage = "";
char device_id[12] = "LoRa 433MHz";
 
void setup() {
  Serial.begin(115200);
  Wire.begin();
 
  // Initialize BH1750 sensor
  lightMeter.begin();
 
  // Initialize BME680 sensor
  if (!bme.begin()) {
    Serial.println("Could not find BME680 sensor, check wiring!");
    return;
  }
 
  // Setup oversampling and filter initialization for BME680 sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
 
  // Initialize LoRa module
  if (!LoRa.begin(433E6)) {
    Serial.println(F("Starting LoRa failed!"));
    while (1);
  }
}
 
void loop() {
  // Perform BME680 sensor reading
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading BME680!");
    return;
  }
  // Variables for BME680 sensor reading
  int temperature = bme.temperature;
  int humidity = bme.humidity;
  int pressure = bme.pressure / 100.0;
  int gas = bme.gas_resistance / 1000.0;
  int dewPoint = dewPointFast(temperature, humidity);
  int brightness = lightMeter.readLightLevel();
  int altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // LoRa message creation
  LoRaMessage = String(device_id) + "/" + String(temperature) + "&" + String(humidity) + "#" + String(pressure) + "@" + String(gas) + "$" + String(dewPoint) + "^" + String(brightness) + "!" + String(altitude);

  //LoRa packet sending
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();

  // Print sensor data to Serial
  Serial.print("Device ID   | ");
  Serial.println(device_id);
  Serial.print("Temperature | ");
  Serial.print(temperature);
  Serial.println(F(" C"));
  Serial.print("Humidity    | ");
  Serial.print(humidity);
  Serial.println(F(" %"));
  Serial.print("Pressure    | ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print("Gas         | ");
  Serial.print(gas);
  Serial.println(" kOhm");
  Serial.print("Dew Point   | ");
  Serial.print(dewPoint);
  Serial.println(" C");
  Serial.print(F("Brightness  | "));
  Serial.print(brightness);
  Serial.println(" lx");
  Serial.print(F("Altitude    | "));
  Serial.print(altitude);
  Serial.println(" m");
  Serial.println();
  delay(transmitInterval);
}
// Dew point calculating
double dewPointFast(double celsius, double humidity)  {
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}
