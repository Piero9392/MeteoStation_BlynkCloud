// Include the SPI (Serial Peripheral Interface) library, which provides functions for communicating with devices using the SPI protocol.
#include <SPI.h>

// Include the Wire library, which provides functions for I2C (Inter-Integrated Circuit) communication.
#include <Wire.h>

// Include the LoRa library, which provides functions for LoRa communication. 
#include <LoRa.h>

// Include the Adafruit Sensor library. This library is used to standardize sensor access across different sensor types.
#include <Adafruit_Sensor.h>

// Include the Adafruit BME680 library, which provides functions to interface with the BME680 sensor.
#include <Adafruit_BME680.h>

// Include the BH1750 library, which provides functions for interfacing with the BH1750 light sensor.
#include <BH1750.h>

// Define a constant named SEALEVELPRESSURE_HPA with a value of 1013.25. This constant represents the standard atmospheric pressure at sea level in hectopascals (hPa). It is used as a reference for pressure measurements to calculate altitude in applications such as barometric sensing.
#define SEALEVELPRESSURE_HPA (1013.25)

// Initialize a BME680 sensor object.
Adafruit_BME680 bme;

// Initialize a BH1750 light sensor object.
BH1750 lightMeter;

// Declare a String variable named 'LoRaMessage' and initialize it with an empty string.This variable will be used to store the message that will be transmitted via LoRa communication.
String LoRaMessage = "";

// Declare a character array named 'device_id' with a size of 12 and initialize it with the string "LoRa 433MHz". This array is intended to store the device ID.
char device_id[12] = "LoRa 433MHz";

void setup() {
  // Begin serial communication at a baud rate of 115200.
  Serial.begin(115200);

  // Initialize the I2C communication bus using the Wire library.
  Wire.begin();

  // Initialize the BH1750 light sensor using the begin() method.
  lightMeter.begin();

  // Check if the BME680 sensor is successfully initialized.
  if (!bme.begin()) {
    // Print an error message if the sensor is not found.
    Serial.println("Could not find BME680 sensor, check wiring!");
  
    // Exit the setup function if the sensor is not found.
    return;
  }

  // Configure BME680 sensor settings for enhanced accuracy and performance.

  // Set temperature oversampling to 8X.
  bme.setTemperatureOversampling(BME680_OS_8X);

  // Set humidity oversampling to 2X.
  bme.setHumidityOversampling(BME680_OS_2X);

  // Set pressure oversampling to 4X.
  bme.setPressureOversampling(BME680_OS_4X);

  // Set the IIR (Infinite Impulse Response) filter size to 3.
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  // Set the gas heater parameters: 320°C for 150 ms.
  bme.setGasHeater(320, 150);

  // Initialize LoRa communication at 433 MHz. Check if LoRa initialization is successful.
  if (!LoRa.begin(433E6)) {
    // Print an error message if LoRa initialization fails.
    Serial.println(F("Starting LoRa failed!"));
    
    // Enter an infinite loop to halt the program if LoRa initialization fails.
    while (1);
  }
}
 
void loop() {
  // Check if the BME680 sensor reading is successful.
  if (!bme.performReading()) {
  // Print an error message to the serial monitor if reading fails.
  Serial.println("Failed to perform reading BME680!");
  
  // Exit the loop function if the reading fails.
  return;
  }

  // Retrieve sensor data from the BME680 sensor and other sensors.

  // Get temperature reading from the BME680 sensor.
  int temperature = bme.temperature;

  // Get humidity reading from the BME680 sensor.
  int humidity = bme.humidity;

  // Get pressure reading from the BME680 sensor and convert to hPa.
  int pressure = bme.pressure / 100.0;

  // Get gas resistance reading from the BME680 sensor and convert to kOhms.
  int gas = bme.gas_resistance / 1000.0;

  // Calculate dew point using the retrieved temperature and humidity.
  int dewPoint = dewPointFast(temperature, humidity);

  // Get light intensity reading from the BH1750 light sensor.
  int brightness = lightMeter.readLightLevel();

  // Get altitude reading from the BME680 sensor using the standard sea-level pressure.
  int altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);


  // Construct a LoRa message containing sensor data. Build the LoRa message using a formatted string, concatenating device ID, temperature, humidity, pressure, gas resistance, dew point, brightness and altitude.
  LoRaMessage = String(device_id) + "/" + String(temperature) + "&" + String(humidity) + "#" + String(pressure) + "@" + String(gas) + "$" + String(dewPoint) + "^" + String(brightness) + "!" + String(altitude);

  // Begin LoRa packet to prepare for transmission.
  LoRa.beginPacket();

  // Print the constructed LoRa message to the transmission buffer.
  LoRa.print(LoRaMessage);

  // Complete the LoRa packet and send the message.
  LoRa.endPacket();

  // Print sensor data to the Serial Monitor for monitoring and debugging.

  // Print device ID.
  Serial.print("Device ID   | ");
  Serial.println(device_id);

  // Print temperature.
  Serial.print("Temperature | ");
  Serial.print(temperature);
  Serial.println(F(" C"));

  // Print humidity.
  Serial.print("Humidity    | ");
  Serial.print(humidity);
  Serial.println(F(" %"));

  // Print pressure.
  Serial.print("Pressure    | ");
  Serial.print(pressure);
  Serial.println(" hPa");

  // Print gas resistance.
  Serial.print("Gas         | ");
  Serial.print(gas);
  Serial.println(" kOhm");

  // Print dew point.
  Serial.print("Dew Point   | ");
  Serial.print(dewPoint);
  Serial.println(" C");

  // Print brightness.
  Serial.print(F("Brightness  | "));
  Serial.print(brightness);
  Serial.println(" lx");

  // Print altitude.
  Serial.print(F("Altitude    | "));
  Serial.print(altitude);
  Serial.println(" m");

  // Print an empty line for better readability.
  Serial.println();

  // Set a delay of 5000 milliseconds (5 seconds) before the next iteration.
  delay(5000);
}

/* Calculate the dew point temperature using the fast approximation formula.
Parameters:
  - celsius: Temperature in degrees Celsius.
  - humidity: Relative humidity as a percentage. */
double dewPointFast(double celsius, double humidity)  {
  // Constants for the dew point calculation. 
  double a = 17.271;
  double b = 237.7;

  // Calculate the temperature at the dew point.
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);

  // Calculate the dew point temperature using the result.
  double Td = (b * temp) / (a - temp);

  // Return the calculated dew point temperature.
  return Td;
}
