/*
Network.cpp
Inkplate Arduino library
David Zovko, Borna Biro, Denis Vajak, Zvonimir Haramustek @ e-radionica.com
September 24, 2020
https://github.com/e-radionicacom/Inkplate-6-Arduino-library
For support, please reach over forums: forum.e-radionica.com/en
For more info about the product, please check: www.inkplate.io
This code is released under the GNU Lesser General Public License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html
Please review the LICENSE file included with this example.
If you have any questions about licensing, please contact techsupport@e-radionica.com
Distributed as-is; no warranty is given.
*/

// Uncomment for American output
// #define AMERICAN

// Network.cpp contains various functions and classes that enable Weather station
// They have been declared in seperate file to increase readability
#include "Network.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>


void Network::begin(char *ssid, char *pass)
{
    // Initiating wifi, like in BasicHttpClient example
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    int cnt = 0;
    Serial.print(F("Waiting for WiFi to connect..."));
    while ((WiFi.status() != WL_CONNECTED))
    {
        Serial.print(F("."));
        delay(1000);
        ++cnt;

        if (cnt == 20)
        {
            Serial.println("Can't connect to WIFI, restarting");
            delay(100);
            ESP.restart();
        }
    }
    Serial.println(F(" connected"));

    // reduce power by making WiFi module sleep
    WiFi.setSleep(1);
}

// Gets time from ntp server
void Network::getTime(char *timeStr)
{
    // Get seconds since 1.1.1970.
    time_t nowSecs = time(nullptr);

    // Used to store time
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);

    // Copies time string into timeStr
    strncpy(timeStr, asctime(&timeinfo) + 11, 5);

    // Setting time string timezone
    int hr = 10 * (timeStr[0] - '0') + (timeStr[1] - '0') + timeZone;

    // Better defined modulo, in case timezone makes hours to go below 0
    hr = (hr % 24 + 24) % 24;

    // Adding time to '0' char makes it into whatever time char, for both digits
    timeStr[0] = hr / 10 + '0';
    timeStr[1] = hr % 10 + '0';
}

void formatTemp(char *str, float temp)
{
    // Converting to Fahrenheit
#ifdef AMERICAN
    temp = (temp * 9.0 / 5.0 + 32.0);
#endif

    // Built in function for float to char* conversion
    dtostrf(temp, 2, 0, str);
}

void formatWind(char *str, float wind)
{
    // Converting to MPH
#ifdef AMERICAN
    wind = wind * 2.237;
#endif

    // Built in function for float to char* conversion
    dtostrf(wind, 2, 0, str);
}

bool Network::getData(char *lon, char *lat, char *apiKey, char *temp1, char *temp2, char *temp3, char *abbr1,
                      char *abbr2, char *abbr3)
{
    bool f = 0;
    // If not connected to wifi reconnect wifi
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();

        delay(5000);

        int cnt = 0;
        Serial.println(F("Waiting for WiFi to reconnect..."));
        while ((WiFi.status() != WL_CONNECTED))
        {
            // Prints a dot every second that wifi isn't connected
            Serial.print(F("."));
            delay(1000);
            ++cnt;

            if (cnt == 7)
            {
                Serial.println("Can't connect to WIFI, restart initiated.");
                delay(100);
                ESP.restart();
            }
        }
    }

    // Wake up if sleeping and save inital state
    bool sleep = WiFi.getSleep();
    WiFi.setSleep(false);

    // Http object used to make get request
    HTTPClient http;

    http.getStream().setNoDelay(true);
    http.getStream().setTimeout(1);

    // Add woeid to api call
    char url[256];
    sprintf(url, "http://api.openweathermap.org/data/2.5/onecall?lat=%s&lon=%s&appid=%s", lat, lon, apiKey);

    // Static Json from ArduinoJson library
    DynamicJsonDocument doc(64000);

    // Initiate http
    http.begin(url);

    // Actually do request
    int httpCode = http.GET();
    if (httpCode == 200)
    {

        // Try parsing JSON object
        DeserializationError error = deserializeJson(doc, http.getStream());

        // If an error happens print it to Serial monitor
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());

            f = 1;
        }
        else
        {
            // Set all data got from internet using formatTemp and formatWind defined above
            // This part relies heavily on ArduinoJson library

            dataEpoch = doc["current"]["dt"].as<time_t>();
            timeZone = doc["timezone_offset"].as<int>() / 3600;

            formatTemp(temp1, doc["hourly"][0]["temp"].as<float>() - 273.15);
            formatTemp(temp2, doc["hourly"][1]["temp"].as<float>() - 273.15);
            formatTemp(temp3, doc["hourly"][2]["temp"].as<float>() - 273.15);

            strlcpy(abbr1, doc["hourly"][0]["weather"][0]["icon"] | "01d", sizeof(abbr1 - 1));
            strlcpy(abbr2, doc["hourly"][1]["weather"][0]["icon"] | "01d", sizeof(abbr2 - 1));
            strlcpy(abbr3, doc["hourly"][2]["weather"][0]["icon"] | "01d", sizeof(abbr3 - 1));

            f = 0;
        }
    }
    else if (httpCode == 401)
    {
        Serial.println("Network error, probably wrong api key");
        while (1)
            ;
    }

    // Stop http and clear document
    doc.clear();
    http.end();

    // Return to initial state
    WiFi.setSleep(sleep);

    return !f;
}

void Network::getHours(char *hour1, char *hour2, char *hour3)
{
    // Format hours info
    sprintf(hour1, "%2ldh", (dataEpoch / 3600L + timeZone + 24) % 24);
    sprintf(hour2, "%2ldh", (dataEpoch / 3600L + 1 + timeZone + 24) % 24);
    sprintf(hour3, "%2ldh", (dataEpoch / 3600L + 2 + timeZone + 24) % 24);
}
