/*
   Inkplate_easyC example for e-radionica.com Inkplate 2
   For this example you will need a micro USB cable, Inkplate 2,
   BME680 sensor with easyC connector on it: https://e-radionica.com/en/bme680-breakout-made-by-e-radionica.html
   and a easyC cable: https://e-radionica.com/en/easyc-cable-20cm.html
   Select "Inkplate 2(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 2(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   This example will show you how you can read temperature, humidity, air pressure and gas data from BME680.
   In order to compile this example successfuly, you will also need to download and install
   Adafruit BME680 library: https://github.com/adafruit/Adafruit_BME680
   and Adafruit Sensor library ( https://github.com/adafruit/Adafruit_Sensor ).
   If you don't know how to install library you can read our tutorial https://e-radionica.com/en/blog/arduino-library/

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   28 March 2022 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE2
#error "Wrong board selection for this example, please select Inkplate 2 in the boards menu."
#endif

#include "Adafruit_BME680.h" //Adafruit library for BME680 Sensor
#include "Inkplate.h"        //Include Inkplate library to the sketch
#include <Adafruit_Sensor.h> //Adafruit library for sensors

Inkplate display; // Create an object on Inkplate library and also set library into 1 Bit mode (BW)
Adafruit_BME680 bme; // Create an object on Adafruit BME680 library
//(with no arguments sent to constructor, that means we are using I2C communication for BME680 sensor)

void setup()
{
  display.begin();        // Init Inkplate library (you should call this function ONLY ONCE)
  display.clearDisplay(); // Clear frame buffer of display
  display.setTextSize(1); // Set text scaling to two (text will be two times bigger than normal)
  display.setTextColor(BLACK, WHITE);

  if (!bme.begin(0x76))
  { // Init. BME680 library. e-radionica.com BME680 sensor board uses 0x76 I2C address for sensor
    display.println("Sensor init failed!");
    display.println("Check sensor wiring/connection!");
    display.display();
    while (1)
      ;
  }

  // Set up oversampling and filter initialization for the sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop()
{
  if (!bme.performReading())
  {
    // If sending command to start reading data fails, send error message to display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Failed to read data from sensor");
    display.display();
  }
  else
  {
    display.setTextColor(BLACK);
    display.clearDisplay(); // Print out new data
    display.setCursor(0, 0);
    display.print("Air temperature: ");
    display.print(bme.temperature);
    display.println(" *C");

    display.setTextColor(RED);

    display.setCursor(0, 25);
    display.print("Air pressure: ");
    display.print(bme.pressure / 100.0);
    display.println(" hPa");

    display.setTextColor(BLACK);

    display.setCursor(0, 50);
    display.print("Air humidity: ");
    display.print(bme.humidity);
    display.println(" %");

    display.setTextColor(RED);

    display.setCursor(0, 75);
    display.print("Gas sensor resistance: ");
    display.print(bme.gas_resistance / 1000.0);
    display.println(" kOhms");

    display.display();
  }
  delay(20000); // Wait a little bit between readings
}
