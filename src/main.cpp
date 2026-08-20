// --------------------------------------------------
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>

TFT_eSPI tft = TFT_eSPI();

PNG png;
#define SD_CS 5
File pngFile;

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

const int DIALOGUE_Y = 170;
const int DIALOGUE_H = 35;

const int STATS_Y = 205;
const int STATS_H = 60;

const int BUTTONS_Y = 265;
const int BUTTONS_H = 55;

unsigned long lastTimeUpdate = 0;

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

void drawPakura(const char* filename);
void *pngOpen(const char *filename, int32_t *size);
void pngClose(void *handle);
int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
int32_t pngSeek(PNGFILE *page, int32_t position);
int pngDraw(PNGDRAW *pDraw);


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

    // Initialise SD card
    if (!SD.begin(SD_CS)) {
        Serial.println("SD CARD FAILED");
    }
    else {
        Serial.println("SD CARD INITIALIZED");
    }

    // Draw the interface
    drawUI();

    Serial.println("PAKURA ONLINE");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    
    // do to:
    // - touchscreen input
    // - character state
    // - animations
    // - stats
    // - dialogue

    // Update the clock once per second
    if (millis() - lastTimeUpdate >= 1000) {

        lastTimeUpdate = millis();

        drawStatusBar();
    }    

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

    // -----------------------------
    // Time
    // -----------------------------

    unsigned long totalSeconds = millis() / 1000;

    int hours = (totalSeconds / 3600) % 24;
    int minutes = (totalSeconds / 60) % 60;

    char timeString[6];

    snprintf(
        timeString,
        sizeof(timeString),
        "%02d:%02d",
        hours,
        minutes
    );

    tft.setCursor(6, 6);
    tft.print(timeString);


    // -----------------------------
    // middle text part
    // -----------------------------

    const char* name = "PAKURA";

    int nameWidth = tft.textWidth(name);

    int nameX = (SCREEN_WIDTH - nameWidth) / 2;

    tft.setCursor(nameX, 6);
    tft.print(name);


    // -----------------------------
    // Level
    // -----------------------------

    tft.setCursor(205, 6);
    tft.print("Lv1");
}

// --------------------------------------------------
// Character area
// --------------------------------------------------

void drawCharacterArea() {

    // Clear character area
    tft.fillRect(
        0,
        CHARACTER_Y,
        SCREEN_WIDTH,
        CHARACTER_H,
        BG_COLOR
    );

    // Draw Pakura's neutral expression
    drawPakura("/pakura/neutral.png");
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

// --------------------------------------------------
// PNG file handling
// --------------------------------------------------

void *pngOpen(const char *filename, int32_t *size) {

    pngFile = SD.open(filename, FILE_READ);

    if (!pngFile) {
        Serial.print("Failed to open PNG: ");
        Serial.println(filename);
        return nullptr;
    }

    *size = pngFile.size();

    return &pngFile;
}


void pngClose(void *handle) {

    File *file = (File *)handle;

    if (file) {
        file->close();
    }
}


int32_t pngRead(
    PNGFILE *page,
    uint8_t *buffer,
    int32_t length
) {

    File *file = (File *)page->fHandle;

    if (!file) {
        return 0;
    }

    return file->read(buffer, length);
}


int32_t pngSeek(
    PNGFILE *page,
    int32_t position
) {

    File *file = (File *)page->fHandle;

    if (!file) {
        return 0;
    }

    return file->seek(position);
}


// --------------------------------------------------
// PNG drawing callback
// --------------------------------------------------

int pngDraw(PNGDRAW *pDraw) {

    uint16_t lineBuffer[240];

    png.getLineAsRGB565(
        pDraw,
        lineBuffer,
        PNG_RGB565_BIG_ENDIAN,
        0xffffffff
    );

    tft.pushImage(
        0,
        CHARACTER_Y + pDraw->y,
        pDraw->iWidth,
        1,
        lineBuffer
    );

    return 1;
}


// --------------------------------------------------
// Draw Pakura expression
// --------------------------------------------------

void drawPakura(const char* filename) {

    Serial.print("Loading expression: ");
    Serial.println(filename);

    int result = png.open(
        filename,
        pngOpen,
        pngClose,
        pngRead,
        pngSeek,
        pngDraw
    );

    if (result != PNG_SUCCESS) {

        Serial.print("PNG ERROR: ");
        Serial.println(result);

        return;
    }

    Serial.print("PNG size: ");
    Serial.print(png.getWidth());
    Serial.print(" x ");
    Serial.println(png.getHeight());

    png.decode(NULL, 0);

    png.close();

    Serial.println("Expression loaded.");
}