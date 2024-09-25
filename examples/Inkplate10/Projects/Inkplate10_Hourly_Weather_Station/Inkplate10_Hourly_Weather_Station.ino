/*
   Inkplate10_Hourly_Weather_Station example for Soldered Inkplate 10
   For this example you will need only USB cable and Inkplate 10.
   Select "e-radionica Inkplate10" or "Soldered Inkplate10" from Tools -> Board menu.
   Don't have "e-radionica Inkplate10" or "Soldered Inkplate10" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   This example will show you how you can use Inkplate 10 to display API data,
   e.g. Metaweather public weather API, and weatherstack for real time data

   IMPORTANT:
   Make sure to change your desired city, timezone and wifi credentials below
   Also have ArduinoJSON installed in your Arduino libraries

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   11 February 2021 by Soldered

   In order to convert your images into a format compatible with Inkplate
   use the Soldered Image Converter available at:
   https://github.com/SolderedElectronics/Soldered-Image-Converter/releases
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(ARDUINO_INKPLATE10) && !defined(ARDUINO_INKPLATE10V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate10 or Soldered Inkplate10 in the boards menu."
#endif

//---------- CHANGE HERE  -------------:

// Time zone for adding hours
int timeZone = 2;

// City name to de displayed on the bottom
char city[128] = "OSIJEK";

// Coordinates sent to the api
char lon[] = "18.5947808";
char lat[] = "45.5510548";

// Change to your wifi ssid and password
char ssid[] = "";
char pass[] = "";

// Uncomment this for MPH and Fahrenheit output, also uncomment it in the begining of Network.cpp
// #define AMERICAN

// Change to your api key, if you don't have one, head over to:
// https://openweathermap.org/guide , register and copy the key provided
char apiKey[] = "";
// Also, declare the function to check if the API key is valid
bool checkIfAPIKeyIsValid(char *APIKEY);

//----------------------------------

// Include Inkplate library to the sketch
#include "Inkplate.h"

// Header file for easier code readability
#include "Network.h"

// Including fonts used
#include "Fonts/Roboto_Light_120.h"
#include "Fonts/Roboto_Light_36.h"
#include "Fonts/Roboto_Light_48.h"

// Including icons generated by the py file
#include "icons.h"

// Delay between API calls, about 1000 per month, which is the free tier limit
#define DELAY_MS 267800L
#define DELAY_WIFI_RETRY_SECONDS 10
#define DELAY_API_RETRY_SECONDS 10

// Inkplate object
Inkplate display(INKPLATE_1BIT);

// All our network functions are in this object, see Network.h
Network network;

// Contants used for drawing icons
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
RTC_DATA_ATTR char temps[4][8] = {
    "-F",
    "-F",
    "-F",
    "-F",
};

// Variables for storing hour strings
RTC_DATA_ATTR char hours[4][8] = {
    "",
    "",
    "",
    "",
};

// Variable for counting partial refreshes
RTC_DATA_ATTR long refreshes = 0;

// Constant to determine when to full update
const int fullRefresh = 10;

// Variables for storing current time and weather info
RTC_DATA_ATTR char currentTemp[16] = "-F";
RTC_DATA_ATTR char currentWind[16] = "-m/s";

RTC_DATA_ATTR char currentTime[16] = "--:--";

RTC_DATA_ATTR char currentWeather[32] = "-";
RTC_DATA_ATTR char currentWeatherAbbr[8] = "01d";

RTC_DATA_ATTR char abbr1[16];
RTC_DATA_ATTR char abbr2[16];
RTC_DATA_ATTR char abbr3[16];
RTC_DATA_ATTR char abbr4[16];

// functions defined below
void formatTime();
void drawWeather();
void drawCurrent();
void drawTemps();
void drawCity();
void drawTime();
void setTime();

void setup()
{
    // Begin serial and display
    Serial.begin(115200);
    display.begin();

    // Initial cleaning of buffer and physical screen
    display.clearDisplay();


    // Connect Inkplate to the WiFi network
    // Try connecting to a WiFi network.
    // Parameters are network SSID, password, timeout in seconds and whether to print to serial.
    // If the Inkplate isn't able to connect to a network stop further code execution and print an error message.
    if (!display.connectWiFi(ssid, pass, WIFI_TIMEOUT, true))
    {
        //Can't connect to netowrk
        // Clear display for the error message
        display.clearDisplay();
        // Set the font size;
        display.setTextSize(3);
        // Set the cursor positions and print the text.
        display.setCursor((display.width() / 2) - 200, display.height() / 2);
        display.print(F("Unable to connect to "));
        display.println(F(ssid));
        display.setCursor((display.width() / 2) - 200, (display.height() / 2) + 30);
        display.println(F("Please check SSID and PASS!"));
        // Display the error message on the Inkplate and go to deep sleep
        display.display();
        esp_sleep_enable_timer_wakeup(1000L * DELAY_WIFI_RETRY_SECONDS);
        (void)esp_deep_sleep_start();
    }

    // Check if we have a valid API key:
    Serial.println("Checking if API key is valid...");
    if (!checkIfAPIKeyIsValid(apiKey))
    {
        // If we don't, notify the user
        Serial.println("API key is invalid!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.println("Can't get data from OpenWeatherMaps! Check your API key!");
        display.println("Only older API keys for OneCall 2.5 work in free tier.");
        display.println("See the code comments the example for more info.");
        display.display();
        // After displaying the error message go to deep sleep
        esp_sleep_enable_timer_wakeup(1000L * DELAY_API_RETRY_SECONDS);
        (void)esp_deep_sleep_start();
    }
    Serial.println("API key is valid!");

    // A function called to format time into a nice string.
    formatTime();

    if (refreshes % fullRefresh == 0)
    {
        while (!network.getData(city, temps[0], temps[1], temps[2], temps[3], currentTemp, currentWind, currentTime,
                                currentWeather, currentWeatherAbbr, abbr1, abbr2, abbr3, abbr4))
        {
            Serial.println("Retrying fetching data!");
            delay(5000);
        }
        network.getHours(hours[0], hours[1], hours[2], hours[3]);
    }

    // Draw data, see functions below for info
    drawWeather();
    drawCurrent();
    drawTemps();
    drawCity();
    drawTime();

    // Refresh full screen every fullRefresh times, defined above
    if (refreshes % fullRefresh == 0)
        display.display();
    else
        display.partialUpdate();

    // Go to sleep before checking again
    ++refreshes;
    esp_sleep_enable_timer_wakeup(1000L * DELAY_MS);
    (void)esp_deep_sleep_start();
}

void loop()
{
}

// Function for getting time from NTP server
void setTime()
{
    // Structure used to hold time information
    struct tm timeInfo;
    time_t nowSec;
    // Fetch current time in epoch format and store it
    display.getNTPEpoch(&nowSec);
    // This loop ensures that the NTP time fetched is valid and beyond a certain threshold
    while (nowSec < 8 * 3600 * 2)
    {
        delay(500);
        yield();
        display.getNTPEpoch(&nowSec);
    }
    gmtime_r(&nowSec, &timeInfo);
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeInfo));
}


/**
 * Make a test API call to see if we have a valid API key
 *
 * Older keys made for OpenWeather 2.5 OneCall API work, while newer ones won't work, due to the service becoming
 * deprecated.
 */
bool checkIfAPIKeyIsValid(char *APIKEY)
{
    bool failed = false;

    // Create the buffer for holding the API response, make it large enough just in case
    char *data;
    data = (char *)ps_malloc(50000LL);

    // Http object used to make GET request
    HTTPClient http;
    http.getStream().setTimeout(10);
    http.getStream().flush();
    http.getStream().setNoDelay(true);

    // Combine the base URL and the API key to do a test call
    char *baseURL = "https://api.openweathermap.org/data/2.5/onecall?lat=45.560001&lon=18.675880&units=metric&appid=";
    char apiTestURL[200];
    sprintf(apiTestURL, "%s%s", baseURL, APIKEY);

    // Begin http by passing url to it
    http.begin(apiTestURL);

    delay(300);

    // Download data until it's a verified complete download
    // Actually do request
    int httpCode = http.GET();

    if (httpCode == 200)
    {
        long n = 0;

        long now = millis();

        while (millis() - now < 1000)
        {
            while (http.getStream().available())
            {
                data[n++] = http.getStream().read();
                now = millis();
            }
        }

        data[n++] = 0;

        // If the reply constains this string - it's invalid
        if (strstr(data, "Invalid API key.") != NULL)
            failed = true;
    }
    else
    {
        // In case there was another HTTP code, it failed
        Serial.print("Error! HTTP Code: ");
        Serial.println(httpCode);
        failed = true;
    }

    // End http
    http.end();

    // Free the memory
    free(data);

    return !failed;
}

void formatTime()
{
    struct tm timeInfo;
    display.getNTPDateTime(&timeInfo);
    time_t nowSec;
    display.getNTPEpoch(&nowSec);
    while(nowSec < 8 * 3600 * 2)
    {
        delay(500);
        yield();
        display.getNTPEpoch(&nowSec);
    }
    gmtime_r(&nowSec, &timeInfo);
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeInfo));

    // Get seconds since 1.1.1970.
    time_t nowSecs = time(nullptr);

    // Used to store time
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);

    // Copies time string into timeStr
    strncpy(currentTime, asctime(&timeinfo) + 11, 5);

    // Setting time string timezone
    int hr = 10 * (currentTime[0] - '0') + (currentTime[1] - '0') + timeZone;

    // Better defined modulo, in case timezone makes hours to go below 0
    hr = (hr % 24 + 24) % 24;

    // Adding time to '0' char makes it into whatever time char, for both digits
    currentTime[0] = hr / 10 + '0';
    currentTime[1] = hr % 10 + '0';
}

// Function for drawing weather info
void drawWeather()
{
    // Searching for weather state abbreviation
    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbrs[i], currentWeatherAbbr) == 0)
            display.drawBitmap(75, 62, logos[i], 152, 152, BLACK);
    }

    // Draw weather state
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);
    display.setCursor(60, 305);
    display.println(currentWeather);
}

// Function for drawing current time
void drawTime()
{
    // Drawing current time
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(1200 - 20 * strlen(currentTime), 35);
    display.println(currentTime);
}

// Function for drawing city name
void drawCity()
{
    // Drawing city name
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(600 - 9 * strlen(city), 795);
    display.println(city);
}

// Function for drawing temperatures
void drawTemps()
{
    // Drawing 4 black rectangles in which temperatures will be written
    int rectWidth = 225;
    int rectSpacing = (1200 - rectWidth * 4) / 5;

    display.fillRect(1 * rectSpacing + 0 * rectWidth, 412, rectWidth, 330, BLACK);
    display.fillRect(2 * rectSpacing + 1 * rectWidth, 412, rectWidth, 330, BLACK);
    display.fillRect(3 * rectSpacing + 2 * rectWidth, 412, rectWidth, 330, BLACK);
    display.fillRect(4 * rectSpacing + 3 * rectWidth, 412, rectWidth, 330, BLACK);

    int textMargin = 6;

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);

    display.setCursor(1 * rectSpacing + 0 * rectWidth + textMargin, 450 + textMargin + 40);
    display.println(hours[0]);

    display.setCursor(2 * rectSpacing + 1 * rectWidth + textMargin, 450 + textMargin + 40);
    display.println(hours[1]);

    display.setCursor(3 * rectSpacing + 2 * rectWidth + textMargin, 450 + textMargin + 40);
    display.println(hours[2]);

    display.setCursor(4 * rectSpacing + 3 * rectWidth + textMargin, 450 + textMargin + 40);
    display.println(hours[3]);

    // Drawing temperature values into black rectangles
    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);

    display.setCursor(1 * rectSpacing + 0 * rectWidth + textMargin, 450 + textMargin + 120);
    display.print(temps[0]);
    display.println(F("C"));

    display.setCursor(2 * rectSpacing + 1 * rectWidth + textMargin, 450 + textMargin + 120);
    display.print(temps[1]);
    display.println(F("C"));

    display.setCursor(3 * rectSpacing + 2 * rectWidth + textMargin, 450 + textMargin + 120);
    display.print(temps[2]);
    display.println(F("C"));

    display.setCursor(4 * rectSpacing + 3 * rectWidth + textMargin, 450 + textMargin + 120);
    display.print(temps[3]);
    display.println(F("C"));

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr1, abbrs[i]) == 0)
            display.drawBitmap(1 * rectSpacing + 0 * rectWidth + textMargin, 450 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr2, abbrs[i]) == 0)
            display.drawBitmap(2 * rectSpacing + 1 * rectWidth + textMargin, 450 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr3, abbrs[i]) == 0)
            display.drawBitmap(3 * rectSpacing + 2 * rectWidth + textMargin, 450 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr4, abbrs[i]) == 0)
            display.drawBitmap(4 * rectSpacing + 3 * rectWidth + textMargin, 450 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }
}

// Current weather drawing function
void drawCurrent()
{
    // Drawing current information

    // Temperature:
    display.setFont(&Roboto_Light_120);
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);

    display.setCursor(365, 206);
    display.print(currentTemp);

    int x = display.getCursorX();
    int y = display.getCursorY();

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);

    display.setCursor(x, y);

#ifdef AMERICAN
    display.println(F("F"));
#else
    display.println(F("C"));
#endif

    // Wind:
    display.setFont(&Roboto_Light_120);
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);

    display.setCursor(720, 206);
    display.print(currentWind);

    x = display.getCursorX();
    y = display.getCursorY();

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);

    display.setCursor(x, y);

#ifdef AMERICAN
    display.println(F("mph"));
#else
    display.println(F("m/s"));
#endif

    // Labels underneath
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(322, 288);
    display.println(F("TEMPERATURE"));

    display.setCursor(750, 288);
    display.println(F("WIND SPEED"));
}
