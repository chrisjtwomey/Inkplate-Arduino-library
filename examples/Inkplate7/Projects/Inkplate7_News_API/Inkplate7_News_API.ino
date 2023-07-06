/*
    Inkplate7_News_API example for Soldered Inkplate 4
    For this example you will need only a USB-C cable and Inkplate 4.
    Select "Soldered Inkplate7" from Tools -> Board menu.
    Don't have "Soldered Inkplate7" option? Follow our tutorial and add it:
    https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

    This example will show you how you can use Inkplate 7 to display API data.
    Here we use News API to get headline news and short description and display
    them on the Inkplate screen. For this you will need an API key which you can obtain
    here: https://newsapi.org/
    On the Serial Monitor at 115200 baud rate, you can see what's happening.

    IMPORTANT:
    Make sure to change your timezone and wifi credentials below.
    Also have ArduinoJson installed in your Arduino libraries, download here: https://arduinojson.org/

    Want to learn more about Inkplate? Visit www.inkplate.io
    Looking to get support? Write on our forums: https://forum.soldered.com/
    29 March 2023 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE7
#error "Wrong board selection for this example, please select Soldered Inkplate7 in the boards menu."
#endif

//---------- CHANGE HERE  -------------:

// Put in your WiFi name (ssid) and password
char ssid[] = "";
char pass[] = "";
char apiKey[] = ""; // You can obtain one here: https://newsapi.org/

// Delay between API calls in miliseconds (first 60 represents minutes so you can change to your need)
// Here is set to 1 call per hour, but if you want to change it, have in mind that in the free plan there are only 100
// free API calls
#define DELAY_MS 60 * 60 * 1000

//-------------------------------------

// Include Inkplate library to the sketch
#include "Inkplate.h"

// Our networking functions, declared in Network.cpp
#include "Network.h"

// Include used fonts
#include "Fonts/GT_Pressura8pt7b.h"
#include "Fonts/Inter6pt7b.h"

// Some settings for the printing in columns
// Adjust if needed
#define COL_WIDTH           200
#define COL_OFFSET          215
#define OFFSET_AFTER_TITLE  20
#define OFFSET_BETWEEN_NEWS 30
#define VERTICAL_OFFSET     12
#define VERTICAL_SPACING    18
#define NUM_COLS            3

// Create object with all networking functions
Network network;

// Create display object
Inkplate display;

void setup()
{
    // Begin serial communitcation, sed for debugging
    Serial.begin(115200);

    // Initial display settings
    display.begin(); // Init Inkplate library (you should call this function ONLY ONCE)
    display.setTextWrap(false);

    // Our begin function
    network.begin(ssid, pass);

    // Pointer to the struct that will hold all news data
    struct news *entities = NULL;

    // Fetch news. If something went wrong the function returns NULL
    entities = network.getData(apiKey);
    if (entities == NULL)
    {
        Serial.println();
        Serial.println("Error fetching news");
        ESP.restart();
    }

    // Draw the news and display it on the screen
    drawNews(entities);
    display.display();

    // Go to sleep before checking again
    esp_sleep_enable_timer_wakeup(1000LL * DELAY_MS); // Set wakeup timer
    (void)esp_deep_sleep_start(); // Put ESP32 into deep sleep (this function returns nothing). Program stops here!
}

void loop()
{
    // Nothing! If you use deep sleep, whole program should be in setup() because each time the board restarts, not in a
    // loop()! loop() must be empty!
}

// Function that draw the news
void drawNews(struct news *entities)
{
    uint8_t coll = 0;                 // For keeping track of columns
    uint16_t y = VERTICAL_OFFSET - 5; // Y coordinate for drawing
    uint8_t rows = 0;                 // For keeping track of rows
    int i = 0;                        // For each piece of news

    // Printing the news until we fill 3 columns
    // If an entire piece of news doesn't fit on the screen, don't print it
    while (coll < NUM_COLS - 1)
    {
        display.setCursor(3 + COL_OFFSET * coll, y); // Set the cursor to the beginning of the current column
        display.setFont(&GT_Pressura8pt7b);          // Set the font for the title
        uint16_t cnt = 0; // Index of each character in the title or description that is printing

        // Let's print the title
        while (*(entities[i].title + cnt) != '\0')
        {
            // Go to the new line if needed
            if (display.getCursorX() > COL_OFFSET * coll + COL_WIDTH ||
                (*(entities[i].title + cnt) == ' ' && display.getCursorX() > COL_OFFSET * coll + COL_WIDTH))
            {
                *(entities[i].title + cnt) == ' ' ? cnt++ : 0;
                rows++;
                y += VERTICAL_SPACING;
                display.setCursor(3 + COL_OFFSET * coll, y);
            }

            // Go to the next column if there is the end of the current one
            if (display.getCursorY() > E_INK_HEIGHT - 5)
            {
                coll++;
                y = VERTICAL_OFFSET;
                display.setCursor(10 + COL_OFFSET * coll, y);
            }

            // Print the text in the frame buffer in before calculated positions
            display.print(*(entities[i].title + cnt));
            cnt++;
        }

        // Move the cursor a bit down and add indentation for the beginning of the sentence
        y = y + OFFSET_AFTER_TITLE;
        display.setCursor(10 + COL_OFFSET * coll, y);
        display.print("  ");

        // Reset the counter
        cnt = 0;

        // Set font for description and print description
        display.setFont(&Inter6pt7b);
        while (*(entities[i].description + cnt) != '\0')
        {
            // Go to the new line (row) if needed
            if (display.getCursorX() > COL_OFFSET * coll + COL_WIDTH ||
                (*(entities[i].description + cnt) == ' ' && display.getCursorX() > COL_OFFSET * coll + COL_WIDTH))
            {
                *(entities[i].description + cnt) == ' ' ? cnt++ : 0;
                rows++;
                y += VERTICAL_SPACING;
                display.setCursor(10 + COL_OFFSET * coll, y);
            }

            // Go to the next column if there is the end of the current one
            if (display.getCursorY() > E_INK_HEIGHT - 5)
            {
                coll++;
                y = VERTICAL_OFFSET + 10;
                display.setCursor(10 + COL_OFFSET * coll, y);
            }

            // Print the text in the frame buffer in before calculated positions
            display.print(*(entities[i].description + cnt));
            cnt++;
        }

        // Add a bit of spacing between 2 news and go to the other piece of news
        y += OFFSET_BETWEEN_NEWS;
        i++;
    }
}
