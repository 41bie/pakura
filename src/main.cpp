// --------------------------------------------------
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>
#include <WiFi.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();

const char* currentExpression = "neutral";

// --------------------------------------------------
// Touchscreen
// --------------------------------------------------

#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25
#define TOUCH_CS   33
#define TOUCH_IRQ  36

#define RAW_X_TOP     340
#define RAW_X_BOTTOM 3641
#define RAW_Y_LEFT   3709
#define RAW_Y_RIGHT   240

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

const int LOG_BUTTON_X = 86;
const int LOG_BUTTON_Y = BUTTONS_Y + 9;
const int LOG_BUTTON_W = 68;
const int LOG_BUTTON_H = 36;

const int MORE_BUTTON_X = 167;
const int MORE_BUTTON_Y = BUTTONS_Y + 9;
const int MORE_BUTTON_W = 68;
const int MORE_BUTTON_H = 36;

const int LOG_MENU_BUTTON_X = 10;
const int LOG_MENU_BUTTON_W = SCREEN_WIDTH - 20;
const int LOG_MENU_BUTTON_H = 38;
const int LOG_MENU_BUTTON_Y = 38;
const int LOG_MENU_BUTTON_GAP = 10;

const int SSID_PAGE_BUTTON_Y = SCREEN_HEIGHT - 38;
const int SSID_PAGE_BUTTON_W = 70;
const int SSID_PAGE_BUTTON_H = 30;
const int SSID_PAGE_LEFT_X = 35;
const int SSID_PAGE_RIGHT_X = SCREEN_WIDTH - 35 - SSID_PAGE_BUTTON_W;

const int BACK_BUTTON_X = 0;
const int BACK_BUTTON_Y = 0;
const int BACK_BUTTON_W = 30;
const int BACK_BUTTON_H = 20;

const int SAT_IMAGE_X = SCREEN_WIDTH - 77;
const int SAT_IMAGE_Y = SCREEN_HEIGHT - 97;

const int MENU_DIALOGUE_X = 0;
const int MENU_DIALOGUE_Y = SCREEN_HEIGHT - 50 - DIALOGUE_H;
const int MENU_DIALOGUE_W = SAT_IMAGE_X;
const int MENU_DIALOGUE_H = DIALOGUE_H;

unsigned long lastTimeUpdate = 0;

bool isLogScreen = false;
bool isMoreScreen = false;
bool isSSIDScreen = false;
bool isStatsScreen = false;
int ssidPage = 0;

// --------------------------------------------------
// WiFi beacon scanning + Stats
// --------------------------------------------------

const unsigned long WIFI_SCAN_INTERVAL = 60000;
const char* SSID_FILE = "/data/ssid.csv";
const char* STATS_FILE = "/stats.json";
const char* STATS_TEMP_FILE = "/stats.json.tmp";

const unsigned long WIFI_EVENT_DURATION = 3000;

unsigned long lastWiFiScan = 0;
bool sdCardReady = false;
String dialogueBeforeWiFiEvent;
String wifiEventDialogue;
unsigned long wifiEventEndsAt = 0;
bool wifiEventDialogueActive = false;

int happiness = 50;
int energy = 60;
int experience = 0;
int pakuraLevel = 1;

// --------------------------------------------------
// Dialogue
// --------------------------------------------------

String currentDialogue = "This is a placeholder message.";

// --------------------------------------------------
// Touch easter egg
// --------------------------------------------------

const unsigned long TOUCH_SEQUENCE_TIMEOUT = 1000;
const unsigned long BLUSH_DURATION = 3500;
const uint8_t REQUIRED_TOUCHES = 3;

uint8_t pakuraTouchCount = 0;
unsigned long lastPakuraTouch = 0;
unsigned long blushStartedAt = 0;
bool isBlushing = false;

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
void drawLogScreen();
void drawMoreScreen();
void drawStatsScreen();
void drawSSIDScreen();
void drawMenuDialogueArea(const char* filename);
void drawBackButton();
void drawLogMenuButton(int y, const char* label);
void drawSSIDPageButton(int x, const char* label);
int loadSSIDPage(int page, int* ids, String* names);
bool loadStats();
bool saveStats();
void updateStats(int newSSIDCount);
void drawStatBar(int y, const char* label, int value);

void setExpression(const char* expression);

void drawPakura(const char* filename);
void drawPng(const char* filename, int imageX, int imageY);
void *pngOpen(const char *filename, int32_t *size);
void pngClose(void *handle);
int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
int32_t pngSeek(PNGFILE *page, int32_t position);
int pngDraw(PNGDRAW *pDraw);

void handleTouch();
void updateBlushState();
bool loadRandomDialogue(const char* filename);
void scanAndStoreSSIDs();
bool appendUniqueSSID(const String& ssid);
void drawCurrentDialogueBox();
void beginWiFiDialogue(const char* filename);
void finishWiFiDialogue(const char* filename);
void updateWiFiDialogue();


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

    // Initialise touchscreen

    pinMode(TOUCH_MOSI, OUTPUT);
    pinMode(TOUCH_MISO, INPUT);
    pinMode(TOUCH_CLK, OUTPUT);
    pinMode(TOUCH_CS, OUTPUT);
    pinMode(TOUCH_IRQ, INPUT);

    digitalWrite(TOUCH_CS, HIGH);
    digitalWrite(TOUCH_CLK, LOW);

    // Portrait orientation for the physically mounted display
    tft.setRotation(1);

    // Clear screen
    tft.fillScreen(BG_COLOR);

    // Initialise SD card
    if (!SD.begin(SD_CS)) {
        Serial.println("SD CARD FAILED");
    }
    else {
        sdCardReady = true;
        Serial.println("SD CARD INITIALIZED");
        randomSeed(micros() ^ analogRead(34));
        loadStats();
        loadRandomDialogue("/dialogue/greeting.txt");
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Draw the interface
    drawUI();

    Serial.println("PAKURA ONLINE");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    if (sdCardReady && millis() - lastWiFiScan >= WIFI_SCAN_INTERVAL) {
        scanAndStoreSSIDs();
    }

    updateWiFiDialogue();

    if (isLogScreen || isMoreScreen || isSSIDScreen || isStatsScreen) {
        handleTouch();
        return;
    }

    // do to:
    // - touchscreen input
    // - character state
    // - animations
    // - stats
    // - dialogue

    // Update the clock once per second (does 1 min currently)
    if (millis() - lastTimeUpdate >= 1000) {

        lastTimeUpdate = millis();

        drawStatusBar();
    }
    handleTouch();    
    updateBlushState();

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
// Log screen
// --------------------------------------------------

void drawLogScreen() {

    tft.fillScreen(BG_COLOR);

    drawLogMenuButton(LOG_MENU_BUTTON_Y, "SSIDs");
    drawLogMenuButton(
        LOG_MENU_BUTTON_Y + LOG_MENU_BUTTON_H + LOG_MENU_BUTTON_GAP,
        "STATS"
    );
    drawPng("/pakura/sat.png", SAT_IMAGE_X, SAT_IMAGE_Y);
    drawMenuDialogueArea("/dialogue/log.txt");
    drawBackButton();
}

void drawStatsScreen() {

    tft.fillScreen(BG_COLOR);

    drawPng("/pakura/sat.png", SAT_IMAGE_X, SAT_IMAGE_Y);
    drawMenuDialogueArea("/dialogue/stats.txt");
    drawBackButton();
}

void drawSSIDScreen() {

    tft.fillScreen(BG_COLOR);

    drawBackButton();

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth("SSIDs")) / 2, 8);
    tft.print("SSIDs");

    int ids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    String names[10];
    int displayedCount = loadSSIDPage(ssidPage, ids, names);

    for (int itemIndex = 0; itemIndex < displayedCount; itemIndex++) {
        int y = 42 + itemIndex * 18;
        tft.setCursor(12, y);
        tft.print(ids[itemIndex]);
        tft.print(". ");
        tft.print(names[itemIndex]);
    }

    drawSSIDPageButton(SSID_PAGE_LEFT_X, "<");
    drawSSIDPageButton(SSID_PAGE_RIGHT_X, ">");
}

void drawBackButton() {

    tft.drawRect(
        BACK_BUTTON_X,
        BACK_BUTTON_Y,
        BACK_BUTTON_W,
        BACK_BUTTON_H,
        LINE_COLOR
    );

    tft.drawLine(20, 6, 10, 10, TEXT_COLOR);
    tft.drawLine(10, 10, 20, 14, TEXT_COLOR);
    tft.drawLine(10, 10, 26, 10, TEXT_COLOR);
}

void drawLogMenuButton(int y, const char* label) {

    tft.drawRect(
        LOG_MENU_BUTTON_X,
        y,
        LOG_MENU_BUTTON_W,
        LOG_MENU_BUTTON_H,
        TEXT_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor(
        (SCREEN_WIDTH - tft.textWidth(label)) / 2,
        y + 15
    );
    tft.print(label);
}

void drawSSIDPageButton(int x, const char* label) {

    tft.drawRect(x, SSID_PAGE_BUTTON_Y, SSID_PAGE_BUTTON_W, SSID_PAGE_BUTTON_H, TEXT_COLOR);
    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(x + 31, SSID_PAGE_BUTTON_Y + 7);
    tft.print(label);
}

// --------------------------------------------------
// More menu screen
// --------------------------------------------------

// --------------------------------------------------
// More screen
// --------------------------------------------------

void drawMoreScreen() {

    tft.fillScreen(BG_COLOR);

    drawPng("/pakura/sat.png", SAT_IMAGE_X, SAT_IMAGE_Y);
    drawMenuDialogueArea("/dialogue/more.txt");

    tft.drawRect(
        BACK_BUTTON_X,
        BACK_BUTTON_Y,
        BACK_BUTTON_W,
        BACK_BUTTON_H,
        LINE_COLOR
    );

    tft.drawLine(20, 6, 10, 10, TEXT_COLOR);
    tft.drawLine(10, 10, 20, 14, TEXT_COLOR);
    tft.drawLine(10, 10, 26, 10, TEXT_COLOR);
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
    tft.print("Lv");
    tft.print(pakuraLevel);
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
    setExpression("neutral");
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

    drawStatBar(STATS_Y + 8, "HAPPINESS", happiness);
    drawStatBar(STATS_Y + 25, "ENERGY", energy);
    drawStatBar(STATS_Y + 42, "EXPERIENCE", experience);
}

void drawStatBar(int y, const char* label, int value) {

    char bar[13] = "[----------]";
    int filled = value / 10;

    for (int index = 0; index < filled; index++) {
        bar[index + 1] = '#';
    }

    tft.setCursor(8, y);
    tft.print(label);
    tft.setCursor(85, y);
    tft.print(bar);
    tft.setCursor(190, y);
    tft.print(value);
}

// --------------------------------------------------
// Main dialogue box
// --------------------------------------------------

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
    tft.print("Pakura:"); //keep this.

    tft.setCursor(8, DIALOGUE_Y + 19);
    tft.print('"');
    tft.print(wifiEventDialogueActive ? wifiEventDialogue : currentDialogue);
    tft.print('"');
}

// --------------------------------------------------
// Secondary-menu dialogue area
// --------------------------------------------------

void drawMenuDialogueArea(const char* filename) {

    loadRandomDialogue(filename);

    tft.fillRect(
        MENU_DIALOGUE_X,
        MENU_DIALOGUE_Y,
        MENU_DIALOGUE_W,
        MENU_DIALOGUE_H,
        BG_COLOR
    );

    tft.drawRect(
        MENU_DIALOGUE_X,
        MENU_DIALOGUE_Y,
        MENU_DIALOGUE_W,
        MENU_DIALOGUE_H,
        LINE_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);

    tft.setCursor(MENU_DIALOGUE_X + 8, MENU_DIALOGUE_Y + 6);
    tft.print("Pakura:");

    tft.setCursor(MENU_DIALOGUE_X + 8, MENU_DIALOGUE_Y + 19);
    tft.print('"');
    tft.print(wifiEventDialogueActive ? wifiEventDialogue : currentDialogue);
    tft.print('"');
}

// --------------------------------------------------
// Main menu buttons
// --------------------------------------------------

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
    const int buttonWidth = LOG_BUTTON_W;
    const int buttonHeight = LOG_BUTTON_H;
    const int buttonY = LOG_BUTTON_Y;

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
        LOG_BUTTON_X,
        buttonY,
        buttonWidth,
        buttonHeight,
        TEXT_COLOR
    );

    tft.setCursor(LOG_BUTTON_X + 14, buttonY + 14);
    tft.print("LOG");

    // Button 3
    tft.drawRect(
        167,
        buttonY,
        buttonWidth,
        buttonHeight,
        TEXT_COLOR
    );

    tft.setCursor(184, buttonY + 14);
    tft.print("MORE");
}

// --------------------------------------------------
// PNG file handling
// --------------------------------------------------

// --------------------------------------------------
// PNG file callbacks
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

// --------------------------------------------------
// PNG render state
// --------------------------------------------------

int pngImageX = 0;
int pngImageY = 0;

int pngDraw(PNGDRAW *pDraw) {

    uint16_t lineBuffer[240];

    png.getLineAsRGB565(
        pDraw,
        lineBuffer,
        PNG_RGB565_BIG_ENDIAN,
        0xffffffff
    );

    tft.pushImage(
        pngImageX,
        pngImageY + pDraw->y,
        pDraw->iWidth,
        1,
        lineBuffer
    );

    return 1;
}

// --------------------------------------------------
// Set Pakura's expression
// --------------------------------------------------

void setExpression(const char* expression) {

    char filename[64];

    snprintf(
        filename,
        sizeof(filename),
        "/pakura/%s.png",
        expression
    );

    Serial.print("Changing expression to: ");
    Serial.println(expression);

    drawPakura(filename);

    currentExpression = expression;
}

// --------------------------------------------------
// Draw Pakura expression
// --------------------------------------------------

void drawPng(const char* filename, int imageX, int imageY) {

    pngImageX = imageX;
    pngImageY = imageY;

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

void drawPakura(const char* filename) {

    drawPng(filename, 0, CHARACTER_Y);
}

// --------------------------------------------------
// Touch input helpers
// --------------------------------------------------

// --------------------------------------------------
// Touchscreen
// --------------------------------------------------

static uint16_t readTouchValue(uint8_t command) {

    uint16_t value = 0;

    for (int bit = 7; bit >= 0; --bit) {

        digitalWrite(
            TOUCH_MOSI,
            command & (1 << bit)
        );

        digitalWrite(TOUCH_CLK, HIGH);
        delayMicroseconds(2);

        digitalWrite(TOUCH_CLK, LOW);
        delayMicroseconds(2);
    }

    // The controller sends one null bit before the 12-bit conversion value.
    digitalWrite(TOUCH_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(TOUCH_CLK, LOW);
    delayMicroseconds(2);

    for (int bit = 11; bit >= 0; --bit) {

        digitalWrite(TOUCH_CLK, HIGH);
        delayMicroseconds(2);

        value |= digitalRead(TOUCH_MISO) << bit;

        digitalWrite(TOUCH_CLK, LOW);
        delayMicroseconds(2);
    }

    return value;
}


static void readTouch(uint16_t &x, uint16_t &y) {

    digitalWrite(TOUCH_CS, LOW);

    x = readTouchValue(0xD0);
    y = readTouchValue(0x90);

    digitalWrite(TOUCH_CS, HIGH);
}


static int clampPixel(long value, int maximum) {

    if (value < 0) {
        return 0;
    }

    if (value > maximum) {
        return maximum;
    }

    return static_cast<int>(value);
}

void handleTouch() {

    if (digitalRead(TOUCH_IRQ) == LOW) {

        uint16_t x = 0;
        uint16_t y = 0;

        readTouch(x, y);

        // Rotation 1 uses the touch controller axes in the opposite screen frame.
        int pixelX = clampPixel(
            map(x, RAW_X_TOP, RAW_X_BOTTOM, 0, 239),
            239
        );

        int pixelY = clampPixel(
            map(y, RAW_Y_LEFT, RAW_Y_RIGHT, 0, 319),
            319
        );

        Serial.print("TOUCH X: ");
        Serial.print(pixelX);

        Serial.print(" Y: ");
        Serial.print(pixelY);

        Serial.print(" (raw X: ");
        Serial.print(x);
        Serial.print(" Y: ");
        Serial.print(y);
        Serial.println(")");

        if (isSSIDScreen) {
            bool touchedBackButton =
                pixelX >= BACK_BUTTON_X &&
                pixelX < BACK_BUTTON_X + BACK_BUTTON_W &&
                pixelY >= BACK_BUTTON_Y &&
                pixelY < BACK_BUTTON_Y + BACK_BUTTON_H;

            bool touchedPreviousPage =
                pixelX >= SSID_PAGE_LEFT_X &&
                pixelX < SSID_PAGE_LEFT_X + SSID_PAGE_BUTTON_W &&
                pixelY >= SSID_PAGE_BUTTON_Y &&
                pixelY < SSID_PAGE_BUTTON_Y + SSID_PAGE_BUTTON_H;

            bool touchedNextPage =
                pixelX >= SSID_PAGE_RIGHT_X &&
                pixelX < SSID_PAGE_RIGHT_X + SSID_PAGE_BUTTON_W &&
                pixelY >= SSID_PAGE_BUTTON_Y &&
                pixelY < SSID_PAGE_BUTTON_Y + SSID_PAGE_BUTTON_H;

            if (touchedBackButton) {
                isSSIDScreen = false;
                isLogScreen = true;
                drawLogScreen();
            }
            else if (touchedPreviousPage && ssidPage > 0) {
                ssidPage--;
                drawSSIDScreen();
            }
            else if (touchedNextPage) {
                int ids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                String names[10];
                if (loadSSIDPage(ssidPage + 1, ids, names) > 0) {
                    ssidPage++;
                    drawSSIDScreen();
                }
            }

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }

        if (isStatsScreen) {
            bool touchedBackButton =
                pixelX >= BACK_BUTTON_X &&
                pixelX < BACK_BUTTON_X + BACK_BUTTON_W &&
                pixelY >= BACK_BUTTON_Y &&
                pixelY < BACK_BUTTON_Y + BACK_BUTTON_H;

            if (touchedBackButton) {
                isStatsScreen = false;
                isLogScreen = true;
                drawLogScreen();
            }

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }

        if (isLogScreen || isMoreScreen) {
            bool touchedBackButton =
                pixelX >= BACK_BUTTON_X &&
                pixelX < BACK_BUTTON_X + BACK_BUTTON_W &&
                pixelY >= BACK_BUTTON_Y &&
                pixelY < BACK_BUTTON_Y + BACK_BUTTON_H;

            if (touchedBackButton) {
                isLogScreen = false;
                isMoreScreen = false;
                loadRandomDialogue("/dialogue/greeting.txt");
                drawUI();
            }

            if (isLogScreen) {
                bool touchedSSIDButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= LOG_MENU_BUTTON_Y &&
                    pixelY < LOG_MENU_BUTTON_Y + LOG_MENU_BUTTON_H;

                bool touchedStatsButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= LOG_MENU_BUTTON_Y + LOG_MENU_BUTTON_H + LOG_MENU_BUTTON_GAP &&
                    pixelY < LOG_MENU_BUTTON_Y + 2 * LOG_MENU_BUTTON_H + LOG_MENU_BUTTON_GAP;

                if (touchedSSIDButton) {
                    isLogScreen = false;
                    isSSIDScreen = true;
                    ssidPage = 0;
                    drawSSIDScreen();
                }
                else if (touchedStatsButton) {
                    isLogScreen = false;
                    isStatsScreen = true;
                    drawStatsScreen();
                }
            }

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }

        bool touchedLogButton =
            pixelX >= LOG_BUTTON_X &&
            pixelX < LOG_BUTTON_X + LOG_BUTTON_W &&
            pixelY >= LOG_BUTTON_Y &&
            pixelY < LOG_BUTTON_Y + LOG_BUTTON_H;

        if (touchedLogButton) {
            isLogScreen = true;
            drawLogScreen();

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }

        bool touchedMoreButton =
            pixelX >= MORE_BUTTON_X &&
            pixelX < MORE_BUTTON_X + MORE_BUTTON_W &&
            pixelY >= MORE_BUTTON_Y &&
            pixelY < MORE_BUTTON_Y + MORE_BUTTON_H;

        if (touchedMoreButton) {
            isMoreScreen = true;
            drawMoreScreen();

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }

        bool touchedCharacter =
            pixelY >= CHARACTER_Y && pixelY < CHARACTER_Y + CHARACTER_H;

        if (!touchedCharacter) {
            pakuraTouchCount = 0;
        }

        if (touchedCharacter && !isBlushing) {
            unsigned long touchTime = millis();

            if (touchTime - lastPakuraTouch >= TOUCH_SEQUENCE_TIMEOUT) {
                pakuraTouchCount = 0;
            }

            lastPakuraTouch = touchTime;
            pakuraTouchCount++;

            if (pakuraTouchCount >= REQUIRED_TOUCHES) {
                setExpression("love"); // using love.png instead of the planned blush.png because blush.png wasnt different enough from neutral.
                loadRandomDialogue("/dialogue/blush.txt");
                drawDialogueArea();
                blushStartedAt = touchTime;
                isBlushing = true;
                pakuraTouchCount = 0;
            }
        }

        while (digitalRead(TOUCH_IRQ) == LOW) {
            delay(10);
        }
    }
}

// --------------------------------------------------
// Blush timer
// --------------------------------------------------

void updateBlushState() {

    if (isBlushing && millis() - blushStartedAt >= BLUSH_DURATION) {
        setExpression("neutral");
        loadRandomDialogue("/dialogue/greeting.txt");
        drawDialogueArea();
        isBlushing = false;
    }
}

// --------------------------------------------------
// Dialogue file loading
// --------------------------------------------------

// --------------------------------------------------
// Random dialogue
// --------------------------------------------------

bool loadRandomDialogue(const char* filename) {

    File dialogueFile = SD.open(filename, FILE_READ);

    if (!dialogueFile) {
        Serial.print("Failed to open dialogue: ");
        Serial.println(filename);
        return false;
    }

    String selectedLine;
    unsigned long lineCount = 0;

    while (dialogueFile.available()) {
        String line = dialogueFile.readStringUntil('\n');
        line.trim();

        if (line.length() == 0) {
            continue;
        }

        lineCount++;

        if (random(lineCount) == 0) {
            selectedLine = line;
        }
    }

    dialogueFile.close();

    if (lineCount == 0) {
        Serial.print("Dialogue is empty: ");
        Serial.println(filename);
        return false;
    }

    currentDialogue = selectedLine;
    return true;
}

// --------------------------------------------------
// Stats persistence
// --------------------------------------------------

bool loadStats() {

    File statsFile = SD.open(STATS_FILE, FILE_READ);

    if (!statsFile) {
        Serial.println("Stats file not found; using defaults");
        return false;
    }

    JsonDocument document;
    DeserializationError error = deserializeJson(document, statsFile);
    statsFile.close();

    if (error) {
        Serial.print("Failed to read stats: ");
        Serial.println(error.c_str());
        return false;
    }

    happiness = constrain(document["happiness"] | 50, 0, 100);
    energy = constrain(document["energy"] | 60, 0, 100);
    experience = constrain(document["xp"] | 0, 0, 99);
    pakuraLevel = max(1, document["level"] | 1);

    Serial.println("Stats loaded");
    return true;
}

bool saveStats() {

    File statsFile = SD.open(STATS_TEMP_FILE, FILE_WRITE);

    if (!statsFile) {
        Serial.println("Failed to open temporary stats file");
        return false;
    }

    JsonDocument document;
    document["happiness"] = happiness;
    document["energy"] = energy;
    document["xp"] = experience;
    document["level"] = pakuraLevel;

    bool writeSucceeded = serializeJson(document, statsFile) > 0;
    statsFile.close();

    if (!writeSucceeded) {
        SD.remove(STATS_TEMP_FILE);
        Serial.println("Failed to write stats");
        return false;
    }

    SD.remove(STATS_FILE);
    if (!SD.rename(STATS_TEMP_FILE, STATS_FILE)) {
        Serial.println("Failed to replace stats file");
        return false;
    }

    Serial.println("Stats saved");
    return true;
}

void updateStats(int newSSIDCount) {

    happiness = constrain(
        happiness + (newSSIDCount > 0 ? 3 : -2),
        0,
        100
    );
    energy = constrain(energy - 1, 0, 100);
    experience += newSSIDCount * 2;

    while (experience >= 100) {
        experience -= 100;
        pakuraLevel++;
    }

    saveStats();

    if (!isLogScreen && !isMoreScreen && !isSSIDScreen && !isStatsScreen) {
        drawStatusBar();
        drawStatsArea();
    }
}

// --------------------------------------------------
// WiFi event dialogue
// --------------------------------------------------

void drawCurrentDialogueBox() {

    if (isSSIDScreen) {
        return;
    }

    if (isLogScreen) {
        drawMenuDialogueArea("/dialogue/log.txt");
    }
    else if (isStatsScreen) {
        drawMenuDialogueArea("/dialogue/stats.txt");
    }
    else if (isMoreScreen) {
        drawMenuDialogueArea("/dialogue/more.txt");
    }
    else {
        drawDialogueArea();
    }
}

void beginWiFiDialogue(const char* filename) {

    dialogueBeforeWiFiEvent = currentDialogue;
    wifiEventDialogueActive = false;

    if (loadRandomDialogue(filename)) {
        wifiEventDialogue = currentDialogue;
        wifiEventDialogueActive = true;
        drawCurrentDialogueBox();
    }
}

void finishWiFiDialogue(const char* filename) {

    if (loadRandomDialogue(filename)) {
        wifiEventDialogue = currentDialogue;
        wifiEventDialogueActive = true;
        wifiEventEndsAt = millis() + WIFI_EVENT_DURATION;
        drawCurrentDialogueBox();
    }
    else {
        wifiEventDialogueActive = false;
    }
}

void updateWiFiDialogue() {

    if (wifiEventDialogueActive &&
        static_cast<long>(millis() - wifiEventEndsAt) >= 0) {
        wifiEventDialogueActive = false;
        currentDialogue = dialogueBeforeWiFiEvent;

        if (!isLogScreen && !isMoreScreen && !isSSIDScreen && !isStatsScreen) {
            setExpression("neutral");
        }

        drawCurrentDialogueBox();
    }
}

// --------------------------------------------------
// WiFi beacon scanning
// --------------------------------------------------

// --------------------------------------------------
// Scan and record networks
// --------------------------------------------------

void scanAndStoreSSIDs() {

    lastWiFiScan = millis();
    bool scanStartedOnMainMenu =
        !isLogScreen && !isMoreScreen && !isSSIDScreen && !isStatsScreen;

    if (scanStartedOnMainMenu) {
        setExpression("surprised");
    }

    beginWiFiDialogue("/dialogue/scan.txt");

    Serial.println("Scanning for SSIDs...");
    int networkCount = WiFi.scanNetworks(false, true);
    int newSSIDCount = 0;
    bool hasNewSSID = false;

    if (networkCount < 0) {
        Serial.println("WiFi scan failed");
    }

    else {
        for (int networkIndex = 0; networkIndex < networkCount; networkIndex++) {
            String ssid = WiFi.SSID(networkIndex);

            if (ssid.length() > 0 && appendUniqueSSID(ssid)) {
                hasNewSSID = true;
                newSSIDCount++;
            }
        }

        WiFi.scanDelete();
    }

    Serial.print("WiFi scan complete: ");
    Serial.print(networkCount);
    Serial.println(" networks found");

    if (scanStartedOnMainMenu) {
        setExpression(hasNewSSID ? "happy" : "sad");
    }

    finishWiFiDialogue(hasNewSSID ? "/dialogue/newssid.txt" : "/dialogue/oldssid.txt");
    updateStats(newSSIDCount);
}

bool appendUniqueSSID(const String& ssid) {

    File ssidFile = SD.open(SSID_FILE, FILE_READ);
    int nextId = 1;

    if (ssidFile) {
        while (ssidFile.available()) {
            String line = ssidFile.readStringUntil('\n');

            if (line.endsWith("\r")) {
                line.remove(line.length() - 1);
            }

            int separatorIndex = line.indexOf(',');

            if (separatorIndex < 0) {
                continue;
            }

            int storedId = line.substring(0, separatorIndex).toInt();
            if (storedId >= nextId) {
                nextId = storedId + 1;
            }

            if (line.substring(separatorIndex + 1) == ssid) {
                ssidFile.close();
                return false;
            }
        }

        ssidFile.close();
    }

    ssidFile = SD.open(SSID_FILE, FILE_APPEND);

    if (!ssidFile) {
        Serial.print("Failed to open SSID file: ");
        Serial.println(SSID_FILE);
        return false;
    }

    ssidFile.print(nextId);
    ssidFile.print(',');
    ssidFile.println(ssid);
    ssidFile.close();

    Serial.print("Stored SSID ");
    Serial.print(nextId);
    Serial.print(": ");
    Serial.println(ssid);
    return true;
}

int loadSSIDPage(int page, int* ids, String* names) {

    File ssidFile = SD.open(SSID_FILE, FILE_READ);

    if (!ssidFile) {
        return 0;
    }

    int recordCount = 0;

    while (ssidFile.available()) {
        String line = ssidFile.readStringUntil('\n');

        if (line.endsWith("\r")) {
            line.remove(line.length() - 1);
        }

        int separatorIndex = line.indexOf(',');
        if (separatorIndex >= 0 && line.substring(separatorIndex + 1).length() > 0) {
            recordCount++;
        }
    }

    const int PAGE_SIZE = 10;
    int firstRecord = recordCount - ((page + 1) * PAGE_SIZE);
    if (firstRecord < 0) {
        firstRecord = 0;
    }

    int lastRecord = recordCount - (page * PAGE_SIZE);
    if (lastRecord > recordCount) {
        lastRecord = recordCount;
    }

    ssidFile.seek(0);
    int recordIndex = 0;
    int selectedIds[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    String selectedNames[10];

    while (ssidFile.available() && recordIndex < lastRecord) {
        String line = ssidFile.readStringUntil('\n');

        if (line.endsWith("\r")) {
            line.remove(line.length() - 1);
        }

        int separatorIndex = line.indexOf(',');
        if (separatorIndex < 0 || line.substring(separatorIndex + 1).length() == 0) {
            continue;
        }

        if (recordIndex >= firstRecord) {
            int slot = recordIndex - firstRecord;
            selectedIds[slot] = line.substring(0, separatorIndex).toInt();
            selectedNames[slot] = line.substring(separatorIndex + 1);
        }

        recordIndex++;
    }

    ssidFile.close();

    int displayedCount = lastRecord - firstRecord;
    for (int itemIndex = 0; itemIndex < displayedCount; itemIndex++) {
        int sourceIndex = displayedCount - 1 - itemIndex;
        ids[itemIndex] = selectedIds[sourceIndex];
        names[itemIndex] = selectedNames[sourceIndex];
    }

    return displayedCount;
}