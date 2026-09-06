// --------------------------------------------------
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "web_server.h"

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
const int BSSID_MENU_BUTTON_Y = LOG_MENU_BUTTON_Y + LOG_MENU_BUTTON_H + LOG_MENU_BUTTON_GAP;
const int STATS_MENU_BUTTON_Y = BSSID_MENU_BUTTON_Y + LOG_MENU_BUTTON_H + LOG_MENU_BUTTON_GAP;

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
const int AP_BUTTON_X = LOG_MENU_BUTTON_X;
const int AP_BUTTON_Y = 30;
const int AP_BUTTON_W = LOG_MENU_BUTTON_W;
const int AP_BUTTON_H = LOG_MENU_BUTTON_H;

#ifdef PAKURA_DEBUG
const int DEBUG_STAT_ROWS[] = {42, 74, 106};
const int DEBUG_MINUS_BUTTON_X = 150;
const int DEBUG_PLUS_BUTTON_X = 180;
const int DEBUG_BUTTON_W = 24;
const int DEBUG_BUTTON_H = 20;
const int DEBUG_TOUCH_PADDING = 6;
#endif

unsigned long lastTimeUpdate = 0;

bool isLogScreen = false;
bool isMoreScreen = false;
bool isSSIDScreen = false;
bool isBSSIDScreen = false;
bool isStatsScreen = false;
#ifdef PAKURA_DEBUG
bool isDebugScreen = false;
bool debugConfirmReset = false;
#endif
bool isInteractScreen = false;
int ssidPage = 0;
bool accessPointEnabled = false;

// --------------------------------------------------
// WiFi beacon scanning + Stats
// --------------------------------------------------

const unsigned long WIFI_SCAN_INTERVAL = 60000;
const char* SSID_FILE = "/data/ssid.csv";
const char* SSID_TEMP_FILE = "/data/ssid.csv.tmp";
const char* STATS_FILE = "/data/stats.json";
const char* STATS_TEMP_FILE = "/data/stats.json.tmp";

const unsigned long WIFI_EVENT_DURATION = 3000;

unsigned long lastWiFiScan = 0;
bool sdCardReady = false;
String dialogueBeforeWiFiEvent;
String wifiEventDialogue;
unsigned long wifiEventEndsAt = 0;
bool wifiEventDialogueActive = false;
bool isSleeping = false;
unsigned long lastSleepRecovery = 0;

const unsigned long CHAT_DURATION = 3000;
unsigned long chatEndsAt = 0;
bool chatActive = false;

int happiness = 50;
int energy = 60;
int experience = 0;
int pakuraLevel = 1;
unsigned long totalUniqueSSIDs = 0;
unsigned long totalScans = 0;
unsigned long totalHappinessGained = 0;
unsigned long totalEnergyGained = 0;
unsigned long totalXPGained = 0;
unsigned long totalHeadpats = 0;

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
#ifdef PAKURA_DEBUG
void drawDebugScreen();
void drawDebugStatRow(int y, const char* label, int value);
void resetStatsToDefault();
#endif
void drawStatsScreen();
void drawStatsSummary();
void drawInteractScreen();
void drawSSIDScreen();
void drawBSSIDScreen();
void drawMenuDialogueArea(const char* filename);
void drawBackButton();
void drawLogMenuButton(int y, const char* label);
void drawSSIDPageButton(int x, const char* label);
int loadSSIDPage(int page, int* ids, String* names);
int loadBSSIDPage(int page, int* ids, String* bssids);
bool loadStats();
bool saveStats();
void updateStats(int newSSIDCount);
void drawStatBar(int y, const char* label, int value);
const char* wifiSecurityName(wifi_auth_mode_t securityType);
bool parseSSIDRecord(
    const String& line,
    int& id,
    String& ssid,
    String& bssid
);

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
bool loadDialogueLine(const char* filename, unsigned int requestedLine);
void scanAndStoreSSIDs();
bool appendUniqueSSID(
    const String& ssid,
    const String& bssid,
    int rssi,
    int channel,
    const String& security
);
void drawCurrentDialogueBox();
void beginWiFiDialogue(const char* filename);
void finishWiFiDialogue(const char* filename);
void updateWiFiDialogue();
void updateChat();
void updateSleep();
void startChat();
void toggleSleep();
void toggleAccessPoint();


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
    beginWebServer();

    // Draw the interface
    drawUI();

    Serial.println("PAKURA ONLINE");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    handleWebServer();

    if (sdCardReady && !isSleeping && millis() - lastWiFiScan >= WIFI_SCAN_INTERVAL) {
        scanAndStoreSSIDs();
    }

    updateWiFiDialogue();
    updateChat();
    updateSleep();

    if (isLogScreen || isMoreScreen || isSSIDScreen || isBSSIDScreen || isStatsScreen || isInteractScreen
#ifdef PAKURA_DEBUG
        || isDebugScreen
#endif
    ) {
        handleTouch();
        return;
    }

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
    drawLogMenuButton(BSSID_MENU_BUTTON_Y, "BBSID (MAC)");
    drawLogMenuButton(STATS_MENU_BUTTON_Y, "STATS");
    drawPng("/pakura/sat.png", SAT_IMAGE_X, SAT_IMAGE_Y);
    drawMenuDialogueArea("/dialogue/log.txt");
    drawBackButton();
}

void drawStatsScreen() {

    tft.fillScreen(BG_COLOR);

    drawStatsSummary();
    drawBackButton();
}

void drawInteractScreen() {

    tft.fillRect(0, BUTTONS_Y, SCREEN_WIDTH, BUTTONS_H, BG_COLOR);
    tft.drawRect(0, BUTTONS_Y, SCREEN_WIDTH, BUTTONS_H, LINE_COLOR);

    const int buttonY = LOG_BUTTON_Y;
    const int buttonWidth = LOG_BUTTON_W;
    const int buttonHeight = LOG_BUTTON_H;
    const int buttonX[] = {5, LOG_BUTTON_X, MORE_BUTTON_X};
    const char* labels[] = {"CHAT", "SLEEP", "BACK"};
    const int textOffsets[] = {12, 13, 14};

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);

    for (int index = 0; index < 3; index++) {
        tft.drawRect(buttonX[index], buttonY, buttonWidth, buttonHeight, TEXT_COLOR);
        tft.setCursor(buttonX[index] + textOffsets[index], buttonY + 14);
        tft.print(labels[index]);
    }
}

void drawStatsSummary() {

    const char* title = "STATS";
    const int firstRowY = 42;
    const int rowHeight = 30;

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth(title)) / 2, 8);
    tft.print(title);

    const char* labels[] = {
        "Total Unique SSIDs:",
        "Total Scans:",
        "Total Happiness Gained:",
        "Total Energy Gained:",
        "Total XP Gained:",
        "Total Headpats:"
    };
    const unsigned long values[] = {
        totalUniqueSSIDs,
        totalScans,
        totalHappinessGained,
        totalEnergyGained,
        totalXPGained,
        totalHeadpats
    };

    for (int index = 0; index < 6; index++) {
        int y = firstRowY + index * rowHeight;
        tft.setCursor(8, y);
        tft.print(labels[index]);
        tft.setCursor(190, y);
        tft.print(values[index]);
    }
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

void drawBSSIDScreen() {

    tft.fillScreen(BG_COLOR);

    drawBackButton();

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth("BBSID (MAC)")) / 2, 8);
    tft.print("BBSID (MAC)");

    int ids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    String bssids[10];
    int displayedCount = loadBSSIDPage(ssidPage, ids, bssids);

    for (int itemIndex = 0; itemIndex < displayedCount; itemIndex++) {
        int y = 42 + itemIndex * 18;
        tft.setCursor(12, y);
        tft.print(ids[itemIndex]);
        tft.print(". ");
        tft.print(bssids[itemIndex]);
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

    const char* accessPointLabel = accessPointEnabled ? "PAKURA WAP: ON" : "PAKURA WAP: OFF";
    drawLogMenuButton(AP_BUTTON_Y, accessPointLabel);
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

#ifdef PAKURA_DEBUG
    const int debugButtonY = MENU_DIALOGUE_Y - LOG_MENU_BUTTON_H - 8;
    tft.drawRect(
        LOG_MENU_BUTTON_X,
        debugButtonY,
        LOG_MENU_BUTTON_W,
        LOG_MENU_BUTTON_H,
        TEXT_COLOR
    );

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor(
        (SCREEN_WIDTH - tft.textWidth("DEBUG")) / 2,
        debugButtonY + 15
    );
    tft.print("DEBUG");
#endif
}

#ifdef PAKURA_DEBUG
void drawDebugStatRow(int y, const char* label, int value) {
    const int labelX = 12;
    const int valueX = 100;

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor(labelX, y + 8);
    tft.print(label);

    tft.setCursor(valueX, y + 8);
    tft.print(value);

    tft.drawRect(DEBUG_MINUS_BUTTON_X, y, DEBUG_BUTTON_W, DEBUG_BUTTON_H, TEXT_COLOR);
    tft.drawRect(DEBUG_PLUS_BUTTON_X, y, DEBUG_BUTTON_W, DEBUG_BUTTON_H, TEXT_COLOR);

    tft.setCursor(DEBUG_MINUS_BUTTON_X + 8, y + 6);
    tft.print("-");
    tft.setCursor(DEBUG_PLUS_BUTTON_X + 8, y + 6);
    tft.print("+");
}

void drawDebugScreen() {
    tft.fillScreen(BG_COLOR);
    drawBackButton();

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth("DEBUG")) / 2, 8);
    tft.print("DEBUG");

    drawDebugStatRow(DEBUG_STAT_ROWS[0], "HAPPINESS", happiness);
    drawDebugStatRow(DEBUG_STAT_ROWS[1], "ENERGY", energy);
    drawDebugStatRow(DEBUG_STAT_ROWS[2], "EXPERIENCE", experience);

    const int resetButtonY = SCREEN_HEIGHT - 42;
    const int resetButtonX = 10;
    const int resetButtonW = SCREEN_WIDTH - 20;
    const int resetButtonH = 28;
    const char* resetLabel = debugConfirmReset ? "CONFIRM?" : "RESET STATS";

    tft.drawRect(resetButtonX, resetButtonY, resetButtonW, resetButtonH, TEXT_COLOR);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth(resetLabel)) / 2, resetButtonY + 9);
    tft.print(resetLabel);
}

void resetStatsToDefault() {
    happiness = 50;
    energy = 60;
    experience = 0;
    pakuraLevel = 1;

    saveStats();
}
#endif

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

    setExpression(isSleeping ? "sleep" : "neutral");
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

    if (isSleeping) {
        loadRandomDialogue("/dialogue/sleep.txt");
    }
    else if (isMoreScreen) {
        loadDialogueLine("/dialogue/more.txt", accessPointEnabled ? 2 : 1);
    }
    else {
        loadRandomDialogue(filename);
    }

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
    tft.print("INTERACT");

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

        if (isSSIDScreen || isBSSIDScreen) {
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
                isBSSIDScreen = false;
                isLogScreen = true;
                drawLogScreen();
            }
            else if (touchedPreviousPage && ssidPage > 0) {
                ssidPage--;
                if (isSSIDScreen) {
                    drawSSIDScreen();
                }
                else {
                    drawBSSIDScreen();
                }
            }
            else if (touchedNextPage) {
                int ids[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                String names[10];
                int nextPageCount = isSSIDScreen
                    ? loadSSIDPage(ssidPage + 1, ids, names)
                    : loadBSSIDPage(ssidPage + 1, ids, names);
                if (nextPageCount > 0) {
                    ssidPage++;
                    if (isSSIDScreen) {
                        drawSSIDScreen();
                    }
                    else {
                        drawBSSIDScreen();
                    }
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

#ifdef PAKURA_DEBUG
        if (isDebugScreen) {
            bool touchedBackButton =
                pixelX >= BACK_BUTTON_X &&
                pixelX < BACK_BUTTON_X + BACK_BUTTON_W &&
                pixelY >= BACK_BUTTON_Y &&
                pixelY < BACK_BUTTON_Y + BACK_BUTTON_H;

            if (touchedBackButton) {
                isDebugScreen = false;
                debugConfirmReset = false;
                isMoreScreen = true;
                drawMoreScreen();
            }
            else {
                for (int index = 0; index < 3; index++) {
                    int y = DEBUG_STAT_ROWS[index];
                    bool touchedMinus =
                        pixelX >= DEBUG_MINUS_BUTTON_X - DEBUG_TOUCH_PADDING &&
                        pixelX < DEBUG_MINUS_BUTTON_X + DEBUG_BUTTON_W + DEBUG_TOUCH_PADDING &&
                        pixelY >= y - DEBUG_TOUCH_PADDING &&
                        pixelY < y + DEBUG_BUTTON_H + DEBUG_TOUCH_PADDING;
                    bool touchedPlus =
                        pixelX >= DEBUG_PLUS_BUTTON_X - DEBUG_TOUCH_PADDING &&
                        pixelX < DEBUG_PLUS_BUTTON_X + DEBUG_BUTTON_W + DEBUG_TOUCH_PADDING &&
                        pixelY >= y - DEBUG_TOUCH_PADDING &&
                        pixelY < y + DEBUG_BUTTON_H + DEBUG_TOUCH_PADDING;

                    if (touchedMinus || touchedPlus) {
                        if (index == 0) {
                            happiness = constrain(happiness + (touchedPlus ? 10 : -10), 0, 100);
                        }
                        else if (index == 1) {
                            energy = constrain(energy + (touchedPlus ? 10 : -10), 0, 100);
                        }
                        else {
                            experience = constrain(experience + (touchedPlus ? 10 : -10), 0, 99);
                        }
                        saveStats();
                        drawDebugScreen();
                        break;
                    }
                }

                bool touchedResetButton =
                    pixelX >= 10 &&
                    pixelX < SCREEN_WIDTH - 10 &&
                    pixelY >= SCREEN_HEIGHT - 42 &&
                    pixelY < SCREEN_HEIGHT - 42 + 28;

                if (touchedResetButton) {
                    if (debugConfirmReset) {
                        resetStatsToDefault();
                        debugConfirmReset = false;
                        drawDebugScreen();
                    }
                    else {
                        debugConfirmReset = true;
                        drawDebugScreen();
                    }
                }
            }

            while (digitalRead(TOUCH_IRQ) == LOW) {
                delay(10);
            }

            return;
        }
#endif

        if (isInteractScreen) {
            if (chatActive) {
                while (digitalRead(TOUCH_IRQ) == LOW) {
                    delay(10);
                }

                return;
            }

            bool touchedChatButton =
                pixelX >= 5 && pixelX < 5 + LOG_BUTTON_W &&
                pixelY >= LOG_BUTTON_Y &&
                pixelY < LOG_BUTTON_Y + LOG_BUTTON_H;

            bool touchedSleepButton =
                pixelX >= LOG_BUTTON_X &&
                pixelX < LOG_BUTTON_X + LOG_BUTTON_W &&
                pixelY >= LOG_BUTTON_Y &&
                pixelY < LOG_BUTTON_Y + LOG_BUTTON_H;

            bool touchedInteractBackButton =
                pixelX >= MORE_BUTTON_X &&
                pixelX < MORE_BUTTON_X + MORE_BUTTON_W &&
                pixelY >= MORE_BUTTON_Y &&
                pixelY < MORE_BUTTON_Y + MORE_BUTTON_H;

            if (touchedChatButton) {
                startChat();
            }
            else if (touchedSleepButton) {
                toggleSleep();
            }
            else if (touchedInteractBackButton) {
                isInteractScreen = false;
                drawButtonArea();
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
                loadRandomDialogue(
                    isSleeping ? "/dialogue/sleep.txt" : "/dialogue/greeting.txt"
                );
                drawUI();
            }

            if (isMoreScreen) {
                bool touchedAccessPointButton =
                    pixelX >= AP_BUTTON_X &&
                    pixelX < AP_BUTTON_X + AP_BUTTON_W &&
                    pixelY >= AP_BUTTON_Y &&
                    pixelY < AP_BUTTON_Y + AP_BUTTON_H;

                if (touchedAccessPointButton) {
                    toggleAccessPoint();
                }

#ifdef PAKURA_DEBUG
                const int debugButtonY = MENU_DIALOGUE_Y - LOG_MENU_BUTTON_H - 8;
                bool touchedDebugButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= debugButtonY &&
                    pixelY < debugButtonY + LOG_MENU_BUTTON_H;

                if (touchedDebugButton) {
                    isMoreScreen = false;
                    isDebugScreen = true;
                    debugConfirmReset = false;
                    drawDebugScreen();
                }
#endif
            }

            if (isLogScreen) {
                bool touchedSSIDButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= LOG_MENU_BUTTON_Y &&
                    pixelY < LOG_MENU_BUTTON_Y + LOG_MENU_BUTTON_H;

                bool touchedBSSIDButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= BSSID_MENU_BUTTON_Y &&
                    pixelY < BSSID_MENU_BUTTON_Y + LOG_MENU_BUTTON_H;

                bool touchedStatsButton =
                    pixelX >= LOG_MENU_BUTTON_X &&
                    pixelX < LOG_MENU_BUTTON_X + LOG_MENU_BUTTON_W &&
                    pixelY >= STATS_MENU_BUTTON_Y &&
                    pixelY < STATS_MENU_BUTTON_Y + LOG_MENU_BUTTON_H;

                if (touchedSSIDButton) {
                    isLogScreen = false;
                    isSSIDScreen = true;
                    isBSSIDScreen = false;
                    ssidPage = 0;
                    drawSSIDScreen();
                }
                else if (touchedBSSIDButton) {
                    isLogScreen = false;
                    isBSSIDScreen = true;
                    isSSIDScreen = false;
                    ssidPage = 0;
                    drawBSSIDScreen();
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

        bool touchedInteractButton =
            pixelX >= 5 &&
            pixelX < 5 + LOG_BUTTON_W &&
            pixelY >= LOG_BUTTON_Y &&
            pixelY < LOG_BUTTON_Y + LOG_BUTTON_H;

        if (touchedInteractButton) {
            isInteractScreen = true;
            drawInteractScreen();

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

        if (touchedCharacter && !isBlushing && !isSleeping) {
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
                totalHeadpats++;
                saveStats();
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
// Interaction actions
// --------------------------------------------------

void startChat() {

    if (isSleeping) {
        isInteractScreen = false;
        drawButtonArea();
        return;
    }

    const char* dialogueFile = "/dialogue/hmid.txt";
    const char* expression = "happy";

    if (happiness <= 9) {
        dialogueFile = "/dialogue/hcritical.txt";
        expression = "cry";
    }
    else if (happiness <= 39) {
        dialogueFile = "/dialogue/hlow.txt";
        expression = "sad";
    }
    else if (happiness >= 80) {
        dialogueFile = "/dialogue/hhigh.txt";
        expression = "laugh";
    }

    loadRandomDialogue(dialogueFile);
    setExpression(expression);
    drawDialogueArea();
    chatEndsAt = millis() + CHAT_DURATION;
    chatActive = true;
}

void toggleSleep() {

    isSleeping = !isSleeping;
    isInteractScreen = false;
    lastSleepRecovery = millis();

    if (isSleeping) {
        loadRandomDialogue("/dialogue/sleep.txt");
    }
    else {
        loadRandomDialogue("/dialogue/greeting.txt");
    }

    drawUI();
}

void toggleAccessPoint() {

    if (accessPointEnabled) {
        disablePakuraAccessPoint();
        accessPointEnabled = false;
    }
    else {
        accessPointEnabled = enablePakuraAccessPoint();
    }

    drawMoreScreen();
}

void updateChat() {

    if (chatActive && static_cast<long>(millis() - chatEndsAt) >= 0) {
        chatActive = false;
        setExpression(isSleeping ? "sleep" : "neutral");
        if (!isSleeping) {
            loadRandomDialogue("/dialogue/greeting.txt");
        }
        drawDialogueArea();
        isInteractScreen = false;
        drawButtonArea();
    }
}

void updateSleep() {

    if (!isSleeping) {
        return;
    }

    while (millis() - lastSleepRecovery >= WIFI_SCAN_INTERVAL) {
        lastSleepRecovery += WIFI_SCAN_INTERVAL;
        energy = constrain(energy + 5, 0, 100);
        totalEnergyGained += 5;
        saveStats();

        if (!isLogScreen && !isMoreScreen && !isSSIDScreen && !isBSSIDScreen && !isStatsScreen) {
            drawStatsArea();
        }
        else if (isStatsScreen) {
            drawStatsSummary();
        }
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

bool loadDialogueLine(const char* filename, unsigned int requestedLine) {

    File dialogueFile = SD.open(filename, FILE_READ);

    if (!dialogueFile) {
        Serial.print("Failed to open dialogue: ");
        Serial.println(filename);
        return false;
    }

    unsigned int lineNumber = 0;
    String selectedLine;

    while (dialogueFile.available()) {
        String line = dialogueFile.readStringUntil('\n');
        line.trim();

        if (line.length() == 0) {
            continue;
        }

        lineNumber++;
        if (lineNumber == requestedLine) {
            selectedLine = line;
            break;
        }
    }

    dialogueFile.close();

    if (selectedLine.length() == 0) {
        Serial.print("Dialogue line not found: ");
        Serial.println(requestedLine);
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
        Serial.print("Stats file not found: ");
        Serial.println(STATS_FILE);
        return false;
    }

    JsonDocument document;
    DeserializationError error = deserializeJson(document, statsFile);
    size_t statsFileSize = statsFile.size();
    statsFile.close();

    if (error) {
        Serial.print("Failed to read stats: ");
        Serial.println(error.c_str());
        return false;
    }

    if (!document["happiness"].is<int>() ||
        !document["energy"].is<int>() ||
        !document["xp"].is<int>() ||
        !document["level"].is<int>()) {
        Serial.println("Stats file is missing a required value; using defaults");
        return false;
    }

    happiness = constrain(document["happiness"].as<int>(), 0, 100);
    energy = constrain(document["energy"].as<int>(), 0, 100);
    experience = constrain(document["xp"].as<int>(), 0, 99);
    pakuraLevel = max(1, document["level"].as<int>());
    totalUniqueSSIDs = document["totalUniqueSSIDs"] | 0UL;
    totalScans = document["totalScans"] | 0UL;
    totalHappinessGained = document["totalHappinessGained"] | 0UL;
    totalEnergyGained = document["totalEnergyGained"] | 0UL;
    totalXPGained = document["totalXPGained"] | 0UL;
    totalHeadpats = document["totalHeadpats"] | 0UL;

    Serial.print("Stats loaded from ");
    Serial.print(STATS_FILE);
    Serial.print(" (");
    Serial.print(statsFileSize);
    Serial.print(" bytes): happiness=");
    Serial.print(happiness);
    Serial.print(", energy=");
    Serial.print(energy);
    Serial.print(", xp=");
    Serial.print(experience);
    Serial.print(", level=");
    Serial.println(pakuraLevel);
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
    document["totalUniqueSSIDs"] = totalUniqueSSIDs;
    document["totalScans"] = totalScans;
    document["totalHappinessGained"] = totalHappinessGained;
    document["totalEnergyGained"] = totalEnergyGained;
    document["totalXPGained"] = totalXPGained;
    document["totalHeadpats"] = totalHeadpats;

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

    totalScans++;
    totalUniqueSSIDs += newSSIDCount;
    totalXPGained += newSSIDCount * 2;

    if (newSSIDCount > 0) {
        totalHappinessGained += 3;
    }

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

    if (!isLogScreen && !isMoreScreen && !isSSIDScreen && !isBSSIDScreen && !isStatsScreen
#ifdef PAKURA_DEBUG
        && !isDebugScreen
#endif
    ) {
        drawStatusBar();
        drawStatsArea();
    }
}

// --------------------------------------------------
// WiFi event dialogue
// --------------------------------------------------

void drawCurrentDialogueBox() {

    if (isSSIDScreen || isBSSIDScreen
#ifdef PAKURA_DEBUG
        || isDebugScreen
#endif
    ) {
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

        if (!isLogScreen && !isMoreScreen && !isSSIDScreen && !isBSSIDScreen && !isStatsScreen
    #ifdef PAKURA_DEBUG
            && !isDebugScreen
    #endif
        ) {
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
        !isLogScreen && !isMoreScreen && !isSSIDScreen && !isBSSIDScreen && !isStatsScreen
#ifdef PAKURA_DEBUG
        && !isDebugScreen
#endif
    ;

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
            if (ssid.length() == 0) {
                ssid = "<hidden>";
            }
            String bssid = WiFi.BSSIDstr(networkIndex);
            int rssi = WiFi.RSSI(networkIndex);
            int channel = WiFi.channel(networkIndex);
            String security = wifiSecurityName(WiFi.encryptionType(networkIndex));

            if (appendUniqueSSID(
                ssid,
                bssid,
                rssi,
                channel,
                security
            )) {
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

const char* wifiSecurityName(wifi_auth_mode_t securityType) {

    switch (securityType) {
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3-PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI-PSK";
        default:
            return "UNKNOWN";
    }
}

bool parseSSIDRecord(
    const String& line,
    int& id,
    String& ssid,
    String& bssid
) {

    int idEnd = line.indexOf(',');
    if (idEnd < 0) {
        return false;
    }

    id = line.substring(0, idEnd).toInt();
    int securitySeparator = line.lastIndexOf(',');
    int channelSeparator = securitySeparator < 0
        ? -1
        : line.lastIndexOf(',', securitySeparator - 1);
    int rssiSeparator = channelSeparator < 0
        ? -1
        : line.lastIndexOf(',', channelSeparator - 1);
    int bssidSeparator = rssiSeparator < 0
        ? -1
        : line.lastIndexOf(',', rssiSeparator - 1);

    if (bssidSeparator > idEnd) {
        ssid = line.substring(idEnd + 1, bssidSeparator);
        bssid = line.substring(bssidSeparator + 1, rssiSeparator);
    }
    else {
        ssid = line.substring(idEnd + 1);
        bssid = "";
    }

    return ssid.length() > 0;
}

bool appendUniqueSSID(
    const String& ssid,
    const String& bssid,
    int rssi,
    int channel,
    const String& security
) {

    File ssidFile = SD.open(SSID_FILE, FILE_READ);
    int nextId = 1;
    bool existingSSID = false;

    if (ssidFile) {
        File tempFile = SD.open(SSID_TEMP_FILE, FILE_WRITE);

        if (!tempFile) {
            ssidFile.close();
            Serial.println("Failed to open temporary SSID file");
            return false;
        }

        while (ssidFile.available()) {
            String line = ssidFile.readStringUntil('\n');

            if (line.endsWith("\r")) {
                line.remove(line.length() - 1);
            }

            int storedId = 0;
            String storedSSID;
            String storedBSSID;

            if (!parseSSIDRecord(line, storedId, storedSSID, storedBSSID)) {
                tempFile.println(line);
                continue;
            }

            if (storedId >= nextId) {
                nextId = storedId + 1;
            }

            if (storedBSSID == bssid && bssid.length() > 0) {
                existingSSID = true;
                tempFile.print(storedId);
                tempFile.print(',');
                tempFile.print(ssid);
                tempFile.print(',');
                tempFile.print(bssid);
                tempFile.print(',');
                tempFile.print(rssi);
                tempFile.print(',');
                tempFile.print(channel);
                tempFile.print(',');
                tempFile.println(security);
            }
            else {
                tempFile.println(line);
            }
        }

        ssidFile.close();
        tempFile.close();

        if (existingSSID) {
            SD.remove(SSID_FILE);
            if (!SD.rename(SSID_TEMP_FILE, SSID_FILE)) {
                Serial.println("Failed to replace SSID file");
                return false;
            }
            return false;
        }

        SD.remove(SSID_TEMP_FILE);
    }

    ssidFile = SD.open(SSID_FILE, FILE_APPEND);

    if (!ssidFile) {
        Serial.print("Failed to open SSID file: ");
        Serial.println(SSID_FILE);
        return false;
    }

    ssidFile.print(nextId);
    ssidFile.print(',');
    ssidFile.print(ssid);
    ssidFile.print(',');
    ssidFile.print(bssid);
    ssidFile.print(',');
    ssidFile.print(rssi);
    ssidFile.print(',');
    ssidFile.print(channel);
    ssidFile.print(',');
    ssidFile.println(security);
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

        int storedId = 0;
        String storedSSID;
        String storedBSSID;

        if (parseSSIDRecord(line, storedId, storedSSID, storedBSSID)) {
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

        int storedId = 0;
        String storedSSID;
        String storedBSSID;

        if (!parseSSIDRecord(line, storedId, storedSSID, storedBSSID)) {
            continue;
        }

        if (recordIndex >= firstRecord) {
            int slot = recordIndex - firstRecord;
            selectedIds[slot] = storedId;
            selectedNames[slot] = storedSSID;
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

int loadBSSIDPage(int page, int* ids, String* bssids) {

    File ssidFile = SD.open(SSID_FILE, FILE_READ);

    if (!ssidFile) {
        return 0;
    }

    const int PAGE_SIZE = 10;
    int recordCount = 0;

    while (ssidFile.available()) {
        String line = ssidFile.readStringUntil('\n');
        line.trim();

        int storedId = 0;
        String storedSSID;
        String storedBSSID;
        if (parseSSIDRecord(line, storedId, storedSSID, storedBSSID)) {
            recordCount++;
        }
    }

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
    int selectedIds[PAGE_SIZE] = {0};
    String selectedBSSIDs[PAGE_SIZE];

    while (ssidFile.available() && recordIndex < lastRecord) {
        String line = ssidFile.readStringUntil('\n');
        line.trim();

        int storedId = 0;
        String storedSSID;
        String storedBSSID;
        if (!parseSSIDRecord(line, storedId, storedSSID, storedBSSID)) {
            continue;
        }

        if (recordIndex >= firstRecord) {
            int slot = recordIndex - firstRecord;
            selectedIds[slot] = storedId;
            selectedBSSIDs[slot] = storedBSSID.length() > 0 ? storedBSSID : "UNKNOWN";
        }

        recordIndex++;
    }

    ssidFile.close();

    int displayedCount = lastRecord - firstRecord;
    for (int itemIndex = 0; itemIndex < displayedCount; itemIndex++) {
        int sourceIndex = displayedCount - 1 - itemIndex;
        ids[itemIndex] = selectedIds[sourceIndex];
        bssids[itemIndex] = selectedBSSIDs[sourceIndex];
    }

    return displayedCount;
}