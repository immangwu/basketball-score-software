/*
  Static "JESUS" Display — P10 LED Matrix
  =========================================
  Library : DMD2 by Freetronics
  Board   : NodeMCU ESP8266

  WIRING (P10 HUB12 to NodeMCU):
  ─────────────────────────────────
  Panel Pin │ Label │ NodeMCU
  ──────────┼───────┼─────────
  Pin  2    │  A    │  D1
  Pin  4    │  B    │  D2
  Pin  6    │  CLK  │  D5
  Pin  1    │  OE   │  D6
  Pin 12    │  DATA │  D7
  Pin  8    │  STB  │  D8
  Pin  3    │  GND  │  GND
  ─────────────────────────────────
  5V Supply (+) → Panel VCC
  5V Supply (–) → Panel GND + NodeMCU GND

  INSTALL DMD2 LIBRARY:
    Sketch → Include Library → Manage Libraries
    Search: DMD2 → Install "DMD2 by Freetronics"
*/

#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial_Black_16.h>

// ── EASY SETTINGS — CHANGE THESE ─────────────────────────────────────

#define DISPLAYS_WIDE   1    // number of panels side by side
#define DISPLAYS_HIGH   1    // number of panels top to bottom

const char* MESSAGE  = "JESUS";   // text to display

// Start pixel (top-left corner of text)
// Panel = 32 cols × 16 rows
// SystemFont5x7 = 5px wide per char × 5 chars + gaps = ~32px wide, 7px tall
// Vertical center = (16 - 7) / 2 = 4
#define START_X   2    // 2px from left edge
#define START_Y   4    // vertically centred on 16-row panel

// Font: choose ONE — uncomment only ONE line
#define USE_FONT  SystemFont5x7      // ← SMALL font 5×7 (fits well on 16 rows)
// #define USE_FONT  Arial_Black_16  // ← LARGE font (too tall for 16 rows)

// Brightness 0–255
#define BRIGHTNESS  255

// ─────────────────────────────────────────────────────────────────────

SoftDMD dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

void setup() {
  Serial.begin(9600);
  dmd.setBrightness(BRIGHTNESS);
  dmd.selectFont(USE_FONT);
  dmd.begin();

  Serial.println("JESUS display ready");
  Serial.print("Text pixel width: ");
  Serial.println(dmd.stringWidth(MESSAGE));

  // Draw static text — stays forever
  dmd.clearScreen();
  dmd.drawString(START_X, START_Y, MESSAGE);
}

void loop() {
  // Nothing needed — DMD2 refreshes automatically
}
