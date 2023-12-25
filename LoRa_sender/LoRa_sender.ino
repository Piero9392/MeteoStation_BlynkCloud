#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme;
#include <BH1750.h> 
BH1750 lightMeter;
String LoRaMessage = "";
char device_id[12] = "LoRa 433MHz";
 
void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  lightMeter.begin();

  if (!bme.begin()) {
    Serial.println("Could not find BME680 sensor, check wiring!");
    return;
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  if (!LoRa.begin(433E6)) {
    Serial.println(F("Starting LoRa FAILED!"));
    while (1);
  }
  LoRa.setSyncWord(0xF3);
}
 
void loop() {
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading BME680!");
    return;
  }
  int temperature = bme.temperature;
  int humidity = bme.humidity;
  int pressure = bme.pressure / 100.0;
  int gas = bme.gas_resistance / 1000.0;
  int dewPoint = dewPointFast(temperature, humidity);
  int brightness = lightMeter.readLightLevel();
  int altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  LoRaMessage = String(device_id) + "/" + String(temperature) + "&" + String(humidity) + "#" + String(pressure) + "@" + String(gas) + "$" + String(dewPoint) + "^" + String(brightness) + "!" + String(altitude);
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();

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
  delay(5000);
}

double dewPointFast(double celsius, double humidity)  {
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}