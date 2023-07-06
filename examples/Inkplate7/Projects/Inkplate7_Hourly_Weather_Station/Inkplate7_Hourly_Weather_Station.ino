/*
   Inkplate7_Hourly_Weather_Station example for Soldered Inkplate 7
   For this example you will need only USB-C cable and Inkplate 7.
   Select "Soldered Inkplate7" from Tools -> Board menu.
   Don't have "Soldered Inkplate7" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   This example will show you how you can use Inkplate 7 to display API data,
   e.g. OpenWeather public weather API for real time data. It shows the forecast
   weather for 4 hours. What happens here is basically ESP32 connects to WiFi and
   sends an API call and the server returns data in JSON format containing data
   about weather, then using the library ArduinoJson we extract icons and temperatures
   per hour from JSON data and show it on Inkplate 7. After displaying the weather,
   ESP32 goes to sleep and wakes up every DELAY_MS milliseconds to show new weather
   (you can change the time interval).

   IMPORTANT:
   Make sure to change your desired city and wifi credentials below.
   Also have ArduinoJson installed in your Arduino libraries:
   https://github.com/bblanchon/ArduinoJson

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   3 Jul 2023 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE7
#error "Wrong board selection for this example, please select Soldered Inkplate7 in the boards menu."
#endif

//---------- CHANGE HERE  -------------:

// City name to be displayed on the bottom
char city[128] = "OSIJEK";

// Coordinates sent to the api
char lon[] = "18.5947808";
char lat[] = "45.5510548";

// Change to your wifi ssid and password
char ssid[] = "";
char pass[] = "";

// Change to your api key, if you don't have one, head over to:
// https://openweathermap.org/guide , register and copy the key provided
char apiKey[] = "";

// Uncomment this for MPH and Fahrenheit output, also uncomment it in the begining of Network.cpp
// #define AMERICAN

//----------------------------------

// Include Inkplate library to the sketch
#include "Inkplate.h"

// Header file for easier code readability
#include "Network.h"

// Including fonts used
#include "Fonts/Inter12pt7b.h"
#include "Fonts/Inter30pt7b.h"
#include "Fonts/Inter8pt7b.h"

// Including icons generated by the py file
#include "icons.h"

// Delay between API calls, about 1000 per month, which is the free tier limit
#define DELAY_MS 267800L

// Inkplate object
Inkplate display;

// All our network functions are in this object, see Network.h
Network network;

// Constants used for drawing icons
char abbrs[32][32] = {"01d", "02d", "03d", "04d", "09d", "10d", "11d", "13d", "50d",
                      "01n", "02n", "03n", "04n", "09n", "10n", "11n", "13n", "50n"};

const uint8_t *logos[18] = {
    icon_01d, icon_02d, icon_03d, icon_04d, icon_09d, icon_10d, icon_11d, icon_13d, icon_50d,
    icon_01n, icon_02n, icon_03n, icon_04n, icon_09n, icon_10n, icon_11n, icon_13n, icon_50n,
};

const uint8_t *s_logos[18] = {
    icon_s_01d, icon_s_02d, icon_s_03d, icon_s_04d, icon_s_09d, icon_s_10d, icon_s_11d, icon_s_13d, icon_s_50d,
    icon_s_01n, icon_s_02n, icon_s_03n, icon_s_04n, icon_s_09n, icon_s_10n, icon_s_11n, icon_s_13n, icon_s_50n,
};

// Variables for storing temperature
char temps[4][8] = {
    "-",
    "-",
    "-",
    "-",
};

// Variables for storing hour strings
char hours[4][8] = {
    "",
    "",
    "",
    "",
};

// Variables for storing current time and weather info
char currentTemp[16] = "-";
char currentWind[16] = "-";

char currentTime[16] = "--:--";

int timeZone;

char currentWeather[32] = "-";
char currentWeatherAbbr[8] = "01d";

char abbr1[16];
char abbr2[16];
char abbr3[16];
char abbr4[16];

// functions defined below
void drawWeather();
void drawCurrent();
void drawTemps();
void drawCity();
void drawTime();

void setup()
{
    // Begin serial and display
    Serial.begin(115200);
    display.begin();

    // Calling our begin from network.h file
    network.begin(ssid, pass);

    // Get all relevant data, see Network.cpp for info
    Serial.print("Fetching data");
    while (!network.getData(lat, lon, apiKey, temps[0], temps[1], temps[2], temps[3], currentTemp, currentWind,
                            currentTime, currentWeather, currentWeatherAbbr, abbr1, abbr2, abbr3, abbr4, &timeZone))
    {
        Serial.print('.');
        delay(500);
    }
    Serial.println();
    network.getTime(currentTime, timeZone);
    network.getHours(hours[0], hours[1], hours[2], hours[3], timeZone);

    // Draw data, see functions below for info
    Serial.println("Drawing on the screen");
    drawWeather();
    drawCurrent();
    drawTemps();
    drawCity();
    drawTime();

    // Next line actually draw all on the screen
    display.display();

    // Go to sleep before checking again
    Serial.println("Going to sleep, bye!");
    esp_sleep_enable_timer_wakeup(1000L * DELAY_MS); // Activate wake-up timer
    (void)esp_deep_sleep_start(); // Start deep sleep (this function does not return). Program stops here.
}

void loop()
{
    // Never here! If you are using deep sleep, the whole program should be in setup() because the board restarts each
    // time. loop() must be empty!
}

// Function for drawing weather info
void drawWeather()
{
    // Searching for weather state abbreviation
    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbrs[i], currentWeatherAbbr) == 0)
            display.drawBitmap(110, 12, logos[i], 96, 96, INKPLATE7_BLACK);
    }

    // Draw weather state
    display.setTextColor(INKPLATE7_BLACK);
    display.setFont(&Inter12pt7b);
    display.setTextSize(1);
    display.setCursor(130, 111);
    display.println(currentWeather);
}

// Function for drawing current time
void drawTime()
{
    // Drawing current time
    display.setTextColor(INKPLATE7_BLACK);
    display.setFont(&Inter12pt7b);
    display.setTextSize(1);

    display.setCursor(E_INK_WIDTH - 14 * strlen(currentTime), 20);
    display.println(currentTime);
}

// Function for drawing city name
void drawCity()
{
    // Drawing city name
    display.setTextColor(INKPLATE7_BLACK);
    display.setFont(&Inter12pt7b);
    display.setTextSize(1);

    display.setCursor(320 - 7 * strlen(city), 360);
    display.println(city);
}

// Draw celsius degrees if AMERICAN isn't defined or Fahrenheit degrees if it's defined
void drawTempUnit()
{
#ifdef AMERICAN
    display.println(F("F"));
#else
    display.println(F("C"));
#endif
}

// Function for drawing temperatures
void drawTemps()
{
    // Drawing 4 rectangles in which temperatures will be written
    int rectWidth = 115;
    int rectSpacing = (E_INK_WIDTH - rectWidth * 4) / 5;
    int yRectangleOffset = 150;
    int rectHeight = 160;

    for (int i = 0; i < 4; i++)
    {
        display.drawRect((i + 1) * rectSpacing + i * rectWidth, yRectangleOffset, rectWidth, rectHeight,
                         INKPLATE7_BLACK);
    }

    int textMargin = 10;

    display.setFont(&Inter8pt7b);
    display.setTextSize(1);
    display.setTextColor(INKPLATE7_BLACK);

    int hourOffset = 10;

    // Drawing hours into rectangles
    for (int i = 0; i < 4; i++)
    {
        display.setCursor((i + 1) * rectSpacing + i * rectWidth + textMargin + 30,
                          yRectangleOffset + textMargin + hourOffset);
        display.println(hours[i]);
    }

    // Drawing temperature values into rectangles
    display.setFont(&Inter12pt7b);
    display.setTextSize(2);
    display.setTextColor(INKPLATE7_RED);

    int tempOffset = 78;

    for (int i = 0; i < 4; i++)
    {
        display.setCursor((i + 1) * rectSpacing + i * rectWidth + textMargin,
                          yRectangleOffset + textMargin + tempOffset);
        display.print(temps[i]);
        drawTempUnit();
    }

    int iconOffset = 93;

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr1, abbrs[i]) == 0)
            display.drawBitmap(1 * rectSpacing + 0 * rectWidth + textMargin + 25, yRectangleOffset + textMargin + iconOffset,
                               s_logos[i], 48, 48, INKPLATE7_BLACK, INKPLATE7_WHITE);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr2, abbrs[i]) == 0)
            display.drawBitmap(2 * rectSpacing + 1 * rectWidth + textMargin + 25, yRectangleOffset + textMargin + iconOffset,
                               s_logos[i], 48, 48, INKPLATE7_BLACK, INKPLATE7_WHITE);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr3, abbrs[i]) == 0)
            display.drawBitmap(3 * rectSpacing + 2 * rectWidth + textMargin + 25, yRectangleOffset + textMargin + iconOffset,
                               s_logos[i], 48, 48, INKPLATE7_BLACK, INKPLATE7_WHITE);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr4, abbrs[i]) == 0)
            display.drawBitmap(4 * rectSpacing + 3 * rectWidth + textMargin + 25, yRectangleOffset + textMargin + iconOffset,
                               s_logos[i], 48, 48, INKPLATE7_BLACK, INKPLATE7_WHITE);
    }
}

// Current weather drawing function
void drawCurrent()
{
    // Drawing current information

    // Temperature:
    display.setFont(&Inter30pt7b);
    display.setTextSize(1);

    display.drawTextWithShadow(285, 90, currentTemp, INKPLATE7_BLACK, INKPLATE7_RED);

    int x = display.getCursorX();
    int y = display.getCursorY();

    display.setFont(&Inter12pt7b);
    display.setTextSize(1);

    display.setCursor(x, y);
    drawTempUnit();

    // Wind:
    display.setFont(&Inter30pt7b);
    display.setTextSize(1);

    display.drawTextWithShadow(460, 90, currentWind, INKPLATE7_BLACK, INKPLATE7_RED);

    x = display.getCursorX();
    y = display.getCursorY();

    display.setFont(&Inter12pt7b);
    display.setTextSize(1);

    display.setCursor(x, y);

#ifdef AMERICAN
    display.println(F("mph"));
#else
    display.println(F("m/s"));
#endif

    // Labels underneath
    display.setFont(&Inter8pt7b);
    display.setTextSize(1);

    display.setCursor(270, 110);
    display.println(F("TEMPERATURE"));

    display.setCursor(450, 110);
    display.println(F("WIND SPEED"));
}
