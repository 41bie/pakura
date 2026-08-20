// --------------------------------------------------
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// --------------------------------------------------
// Screen dimensions
// --------------------------------------------------

const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;

// --------------------------------------------------
// UI areas
// --------------------------------------------------

const int STATUS_Y = 0;
const int STATUS_H = 20;

const int CHARACTER_Y = 20;
const int CHARACTER_H = 150;

const int STATS_Y = 170;
const int STATS_H = 60;

const int DIALOGUE_Y = 230;
const int DIALOGUE_H = 35;

const int BUTTONS_Y = 265;
const int BUTTONS_H = 55;

// --------------------------------------------------
// Colours
// --------------------------------------------------

const uint16_t BG_COLOR = TFT_BLACK;
const uint16_t TEXT_COLOR = TFT_WHITE;
const uint16_t LINE_COLOR = TFT_DARKGREY;

// --------------------------------------------------
// Function declarations
// --------------------------------------------------

void drawUI();
void drawStatusBar();
void drawCharacterArea();
void drawStatsArea();
void drawDialogueArea();
void drawButtonArea();


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

    Serial.begin(115200);

    // Turn display backlight on
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    // Initialise display
    tft.init();

    // Portrait orientation
    tft.setRotation(1);

    // Clear screen
    tft.fillScreen(BG_COLOR);

    // Draw the interface
    drawUI();

    Serial.println("PAKURA ONLINE");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    // Nothing here yet.
    // Eventually this will handle:
    // - touchscreen input
    // - character state
    // - animations
    // - stats
    // - dialogue
}

// --------------------------------------------------
// Draw complete UI
// --------------------------------------------------

void drawUI() {

    drawStatusBar();
    drawCharacterArea();
    drawStatsArea();
    drawDialogueArea();
    drawButtonArea();
}

// --------------------------------------------------
// Status bar
// --------------------------------------------------

void drawStatusBar() {

    tft.fillRect(
        0,
        STATUS_Y,
        SCREEN_WIDTH,
        STATUS_H,
        BG_COLOR
    );

    tft.drawRect(
        0,
        STATUS_Y,
        SCREEN_WIDTH,
        STATUS_H,
        LINE_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);

    tft.setCursor(6, 6);
    tft.print("time");

    tft.setCursor(80, 6);
    tft.print("PAKURA");

    tft.setCursor(205, 6);
    tft.print("Lv1");
}

// --------------------------------------------------
// Character area
// --------------------------------------------------

void drawCharacterArea() {

    tft.fillRect(
        0,
        CHARACTER_Y,
        SCREEN_WIDTH,
        CHARACTER_H,
        BG_COLOR
    );

    tft.drawRect(
        0,
        CHARACTER_Y,
        SCREEN_WIDTH,
        CHARACTER_H,
        LINE_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(2);

    tft.setCursor(60, CHARACTER_Y + 65);
    tft.print("[ CHARACTER ]");

    tft.setTextSize(1);

    tft.setCursor(72, CHARACTER_Y + 95);
    tft.print("240 x 150");
}

// --------------------------------------------------
// Stats area
// --------------------------------------------------

void drawStatsArea() {

    tft.fillRect(
        0,
        STATS_Y,
        SCREEN_WIDTH,
        STATS_H,
        BG_COLOR
    );

    tft.drawRect(
        0,
        STATS_Y,
        SCREEN_WIDTH,
        STATS_H,
        LINE_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);

    // Happiness
    tft.setCursor(8, STATS_Y + 8);
    tft.print("HAPPINESS");

    tft.setCursor(85, STATS_Y + 8);
    tft.print("[########--]");

    tft.setCursor(190, STATS_Y + 8);
    tft.print("80");

    // Energy
    tft.setCursor(8, STATS_Y + 25);
    tft.print("ENERGY");

    tft.setCursor(85, STATS_Y + 25);
    tft.print("[######----]");

    tft.setCursor(190, STATS_Y + 25);
    tft.print("60");

    // XP
    tft.setCursor(8, STATS_Y + 42);
    tft.print("EXPERIENCE");

    tft.setCursor(85, STATS_Y + 42);
    tft.print("[###-------]");

    tft.setCursor(190, STATS_Y + 42);
    tft.print("30");
}

// --------------------------------------------------
// Dialogue area
// --------------------------------------------------

void drawDialogueArea() {

    tft.fillRect(
        0,
        DIALOGUE_Y,
        SCREEN_WIDTH,
        DIALOGUE_H,
        BG_COLOR
    );

    tft.drawRect(
        0,
        DIALOGUE_Y,
        SCREEN_WIDTH,
        DIALOGUE_H,
        LINE_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);

    tft.setCursor(8, DIALOGUE_Y + 6);
    tft.print("Pakura:");

    tft.setCursor(8, DIALOGUE_Y + 19);
    tft.print("\"This is a placeholder message.\"");
}

// --------------------------------------------------
// Button area
// --------------------------------------------------

void drawButtonArea() {

    tft.fillRect(
        0,
        BUTTONS_Y,
        SCREEN_WIDTH,
        BUTTONS_H,
        BG_COLOR
    );

    tft.drawRect(
        0,
        BUTTONS_Y,
        SCREEN_WIDTH,
        BUTTONS_H,
        LINE_COLOR
    );

    // Button dimensions
    const int buttonWidth = 68;
    const int buttonHeight = 36;
    const int buttonY = BUTTONS_Y + 9;

    // Button 1
    tft.drawRect(
        5,
        buttonY,
        buttonWidth,
        buttonHeight,
        TEXT_COLOR
    );

    tft.setCursor(18, buttonY + 14);
    tft.print("TALK");

    // Button 2
    tft.drawRect(
        86,
        buttonY,
        buttonWidth,
        buttonHeight,
        TEXT_COLOR
    );

    tft.setCursor(100, buttonY + 14);
    tft.print("STATUS");

    // Button 3
    tft.drawRect(
        167,
        buttonY,
        buttonWidth,
        buttonHeight,
        TEXT_COLOR
    );

    tft.setCursor(184, buttonY + 14);
    tft.print("DATA");
}