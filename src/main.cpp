// --------------------------------------------------
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>

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

const int BACK_BUTTON_X = 0;
const int BACK_BUTTON_Y = 0;
const int BACK_BUTTON_W = 30;
const int BACK_BUTTON_H = 20;

unsigned long lastTimeUpdate = 0;

bool isLogScreen = false;
bool isMoreScreen = false;

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

void setExpression(const char* expression);

void drawPakura(const char* filename);
void *pngOpen(const char *filename, int32_t *size);
void pngClose(void *handle);
int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
int32_t pngSeek(PNGFILE *page, int32_t position);
int pngDraw(PNGDRAW *pDraw);

void handleTouch();
void updateBlushState();
bool loadRandomDialogue(const char* filename);


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
        Serial.println("SD CARD INITIALIZED");
        randomSeed(micros() ^ analogRead(34));
        loadRandomDialogue("/dialogue/greeting.txt");
    }

    // Draw the interface
    drawUI();

    Serial.println("PAKURA ONLINE");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

    if (isLogScreen || isMoreScreen) {
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
// More screen
// --------------------------------------------------

void drawMoreScreen() {

    tft.fillScreen(BG_COLOR);

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
    tft.print("Pakura:"); //keep this.

    tft.setCursor(8, DIALOGUE_Y + 19);
    tft.print('"');
    tft.print(currentDialogue);
    tft.print('"');
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

        if (isLogScreen || isMoreScreen) {
            bool touchedBackButton =
                pixelX >= BACK_BUTTON_X &&
                pixelX < BACK_BUTTON_X + BACK_BUTTON_W &&
                pixelY >= BACK_BUTTON_Y &&
                pixelY < BACK_BUTTON_Y + BACK_BUTTON_H;

            if (touchedBackButton) {
                isLogScreen = false;
                isMoreScreen = false;
                drawUI();
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