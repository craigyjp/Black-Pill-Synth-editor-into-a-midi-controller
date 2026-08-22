#include <Adafruit_GFX.h>
#include <ST7735_t3.h>  // Hardware-specific library
#include <ST7789_t3.h>  // Hardware-specific library

#include <Fonts/Org_01.h>
#include "Yeysk16pt7b.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansOblique24pt7b.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>

// #define cs 38
// #define dc 3
// #define rst 9

#define DISPLAYTIMEOUT 1500

#define PULSE 1
#define VAR_TRI 2
#define FILTER_ENV 3
#define AMP_ENV 4

ST7789_t3 tft = ST7789_t3(10, 3, 11, 13, 2);

String currentPerfNum = "";
String currentPerfName = "";
String currentPgmNum = "";
String currentPatchName = "";
int currentUpperPatchNo = 0;
String currentUpperPatchName = "";
int currentLowerPatchNo = 0;
String currentLowerPatchName = "";

String currentParameter = "";
String currentValue = "";
float currentFloatValue = 0.0;
String currentPgmNumU = "";
String currentPgmNumL = "";
String currentPatchNameU = "";
String currentPatchNameL = "";
String newPatchName = "";
String savePatchName = "";
const char *currentSettingsOption = "";
const char *currentSettingsValue = "";
int currentSettingsPart = SETTINGS;
int paramType = PARAMETER;

//boolean voiceOn[NO_OF_VOICES] = { false };
boolean MIDIClkSignal = false;

unsigned long timer = 0;

void startTimer() {
  if (state == PARAMETER) {
    timer = millis();
  }
}

void renderBootUpPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.drawRect(42, 30, 46, 11, ST7735_WHITE);
  tft.fillRect(88, 30, 61, 11, ST7735_WHITE);
  tft.setCursor(45, 31);
  tft.setFont(&Org_01);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.println("ACB Tech");
  tft.setTextColor(ST7735_BLACK);
  tft.setCursor(91, 37);
  tft.println("SYNTHESIZER");
  tft.setTextColor(ST7735_YELLOW);
  tft.setFont(&Yeysk16pt7b);
  tft.setCursor(0, 70);
  tft.setTextSize(1);
  tft.println("Blue Pill");
  tft.setTextColor(ST7735_RED);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(110, 95);
  tft.println(VERSION);
}

void renderCurrentPatchPage() {
  tft.fillScreen(ST7735_BLACK);

  // ─── Section Dividers ──────────────────────
  tft.drawFastHLine(0, 50, tft.width(), ST7735_RED);
  tft.drawFastHLine(0, 140, tft.width(), ST7735_RED);

  // ─── Header Line: Num / Label / Mode ───────
  tft.setTextColor(ST7735_YELLOW);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(1);


  tft.setCursor(5, 20);
  tft.println("Num");

  tft.setCursor(70, 20);
  tft.println("Patchname");

  tft.setCursor(240, 20);


  // Lower patch block (always shown)
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(0, 170);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(currentPgmNum);

  tft.setCursor(90, 175);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.println(currentPatchName);
}

void renderCurrentParameterPage() {
  tft.fillScreen(ST7735_BLACK);

  // ─── Section Dividers ──────────────────────
  tft.drawFastHLine(0, 50, tft.width(), ST7735_RED);
  tft.drawFastHLine(0, 140, tft.width(), ST7735_RED);

  // ─── Header Line: Num / Label / Mode ───────
  tft.setTextColor(ST7735_YELLOW);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(1);


  tft.setCursor(5, 20);
  tft.println("Num");


  tft.setCursor(70, 20);
  tft.println("Patchname");

  tft.setCursor(240, 20);  // top-right corner

  // ─── Patch Display ─────────────────────────

  // Lower patch (always shown)
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(0, 170);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(currentPgmNum);

  tft.setCursor(90, 175);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.println(currentPatchName);

  // ─── Parameter Display (erase before draw) ─

  tft.fillRect(0, 60, tft.width(), 80, ST7735_BLACK);  // FIX: covers upper section fully
  tft.setCursor(0, 70);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(2);
  tft.println(currentParameter);

  tft.setCursor(0, 100);
  tft.setTextColor(ST7735_WHITE);
  tft.println(currentValue);
}

void renderSavePage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Save?");
  tft.drawFastHLine(10, 50, tft.width() - 20, ST7735_RED);

  tft.setTextSize(2);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(10, 80);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches[patches.size() - 2].patchNo);
  tft.setCursor(100, 80);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches[patches.size() - 2].patchName);

  tft.fillRect(10, 120, tft.width() - 20, 44, ST77XX_RED);

  tft.setCursor(10, 130);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches.last().patchNo);
  tft.setCursor(100, 130);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches.last().patchName);
}

void renderReinitialisePage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Initialise to");
  tft.setCursor(10, 80);
  tft.println("panel settings");
}

void renderPatchNamingPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Rename Patch");
  tft.drawFastHLine(10, 50, tft.width() - 20, ST7735_RED);

  tft.setTextSize(2);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 80);
  tft.println(newPatchName);
}

void renderPatchSavingPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Save Patch");
  tft.drawFastHLine(10, 50, tft.width() - 20, ST7735_RED);

  tft.setTextSize(2);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 80);
  tft.println(savePatchName);
}

void showRenamingPage(String newName) {
  newPatchName = newName;
}

void showSavingPage(String newName) {
  savePatchName = newName;
}

void renderUpDown(uint16_t x, uint16_t y, uint16_t colour) {
  //Produces up/down indicator glyph at x,y
  tft.setCursor(x, y);
  tft.fillTriangle(x, y, x + 8, y - 8, x + 16, y, colour);
  tft.fillTriangle(x, y + 4, x + 8, y + 12, x + 16, y + 4, colour);
}

void renderSettingsPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Settings");
  tft.drawFastHLine(10, 50, tft.width() - 20, ST7735_RED);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.println(currentSettingsOption);
  if (currentSettingsPart == SETTINGS) renderUpDown(240, 90, ST7735_YELLOW);
  tft.drawFastHLine(10, 125, tft.width() - 20, ST7735_RED);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 150);
  tft.println(currentSettingsValue);
  if (currentSettingsPart == SETTINGSVALUE) renderUpDown(240, 160, ST7735_WHITE);
}

void renderRecallPage()
{
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(0, 45);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches.last().patchNo);
  tft.setCursor(35, 45);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches.last().patchName);

  tft.fillRect(0, 56, tft.width(), 23, 0xA000);
  tft.setCursor(0, 72);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches.first().patchNo);
  tft.setCursor(35, 72);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches.first().patchName);

  tft.setCursor(0, 98);
  tft.setTextColor(ST7735_YELLOW);
  patches.size() > 1 ? tft.println(patches[1].patchNo) : tft.println(patches.last().patchNo);
  tft.setCursor(35, 98);
  tft.setTextColor(ST7735_WHITE);
  patches.size() > 1 ? tft.println(patches[1].patchName) : tft.println(patches.last().patchName);
}

void renderDeletePatchPage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(5, 53);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Delete?");
  tft.drawFastHLine(10, 60, tft.width() - 20, ST7735_RED);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(0, 78);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches.last().patchNo);
  tft.setCursor(35, 78);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches.last().patchName);
  tft.fillRect(0, 85, tft.width(), 23, ST7735_RED);
  tft.setCursor(0, 98);
  tft.setTextColor(ST7735_YELLOW);
  tft.println(patches.first().patchNo);
  tft.setCursor(35, 98);
  tft.setTextColor(ST7735_WHITE);
  tft.println(patches.first().patchName);
}

void renderDeleteMessagePage() {
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(2, 53);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.println("Renumbering");
  tft.setCursor(10, 90);
  tft.println("SD Card");
}

void showCurrentParameterPage(const char *param, float val, int pType) {
  currentParameter = param;
  currentValue = String(val);
  currentFloatValue = val;
  paramType = pType;
  startTimer();
}

void showCurrentParameterPage(const char *param, String val, int pType) {
  if (state == SETTINGS || state == SETTINGSVALUE) state = PARAMETER;  //Exit settings page if showing
  currentParameter = param;
  currentValue = val;
  paramType = pType;
  startTimer();
}

void showCurrentParameterPage(const char *param, String val) {
  showCurrentParameterPage(param, val, PARAMETER);
}

void showPatchPage(String number, String patchName) {
  currentPgmNum = number;
  currentPatchName = patchName;
}

void showSettingsPage(const char *option, const char *value, int settingsPart) {
  currentSettingsOption = option;
  currentSettingsValue = value;
  currentSettingsPart = settingsPart;
}

// ---------- Screen renderer ----------
void updateScreen() {
  switch (state) {
    case PARAMETER:
      if ((millis() - timer) > DISPLAYTIMEOUT) renderCurrentPatchPage();
      else renderCurrentParameterPage();
      break;

    case RECALL:
      renderRecallPage();
      break;

    case SAVE:
      renderSavePage();
      break;

    case REINITIALISE:
      renderReinitialisePage();
      tft.updateScreen();
      state = PARAMETER;
      break;

    case PATCH:
      renderCurrentPatchPage();
      break;

    case PATCHNAMING:
      renderPatchNamingPage();
      break;

    case DELETE:
      renderDeletePatchPage();
      break;

    case DELETEMSG:
      renderDeleteMessagePage();
      break;

    case SETTINGS:
    case SETTINGSVALUE:
      renderSettingsPage();
      break;
  }

  tft.updateScreen();
}

void setupDisplay() {
  tft.init(240, 320);
  tft.useFrameBuffer(true);
  tft.setRotation(1);
  tft.invertDisplay(true);
  tft.fillScreen(ST7735_BLACK);
  renderBootUpPage();
  tft.updateScreen();
}
