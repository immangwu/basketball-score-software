/*
  P10 Board Hardware Test — DMD2 Library
  ========================================
  Tests the panel step by step.
  Watch Serial Monitor at 9600 baud for status.

  Board   : Arduino UNO
  Library : DMD2 by Freetronics

  WIRING:
    Panel CLK  → Arduino Pin 13
    Panel DATA → Arduino Pin 11
    Panel STB  → Arduino Pin 9
    Panel OE   → Arduino Pin 6
    Panel A    → Arduino Pin 7
    Panel B    → Arduino Pin 8
    Panel GND  → Arduino GND + 5V Supply GND
    Panel VCC  → 5V Supply (+)
*/

#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>

SoftDMD dmd(1, 1);

void setup() {
  Serial.begin(9600);
  dmd.setBrightness(255);
  dmd.begin();
  Serial.println("=== P10 Board Test ===");
}

int testNum = 0;
unsigned long lastTest = 0;

void loop() {
  if (millis() - lastTest < 2500) return;
  lastTest = millis();

  dmd.clearScreen();

  switch (testNum) {

    case 0:
      // ALL LEDs ON — entire panel should glow
      Serial.println("TEST 1: ALL LEDs ON — full panel bright?");
      for (int x = 0; x < 32; x++)
        for (int y = 0; y < 16; y++)
          dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 1:
      // ALL LEDs OFF — panel should be blank
      Serial.println("TEST 2: ALL LEDs OFF — panel blank?");
      dmd.clearScreen();
      break;

    case 2:
      // TOP HALF only (rows 0–7)
      Serial.println("TEST 3: TOP HALF rows 0-7 ON");
      for (int x = 0; x < 32; x++)
        for (int y = 0; y < 8; y++)
          dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 3:
      // BOTTOM HALF only (rows 8–15)
      Serial.println("TEST 4: BOTTOM HALF rows 8-15 ON");
      for (int x = 0; x < 32; x++)
        for (int y = 8; y < 16; y++)
          dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 4:
      // LEFT HALF only (cols 0–15)
      Serial.println("TEST 5: LEFT HALF cols 0-15 ON");
      for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
          dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 5:
      // RIGHT HALF only (cols 16–31)
      Serial.println("TEST 6: RIGHT HALF cols 16-31 ON");
      for (int x = 16; x < 32; x++)
        for (int y = 0; y < 16; y++)
          dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 6:
      // CHECKERBOARD
      Serial.println("TEST 7: CHECKERBOARD pattern");
      for (int x = 0; x < 32; x++)
        for (int y = 0; y < 16; y++)
          if ((x + y) % 2 == 0) dmd.setPixel(x, y, GRAPHICS_ON);
      break;

    case 7:
      // BORDER only
      Serial.println("TEST 8: BORDER (outer edge only)");
      for (int x = 0; x < 32; x++) {
        dmd.setPixel(x, 0,  GRAPHICS_ON);
        dmd.setPixel(x, 15, GRAPHICS_ON);
      }
      for (int y = 0; y < 16; y++) {
        dmd.setPixel(0,  y, GRAPHICS_ON);
        dmd.setPixel(31, y, GRAPHICS_ON);
      }
      break;

    case 8:
      // Display "OK" text
      Serial.println("TEST 9: Display text OK");
      dmd.selectFont(SystemFont5x7);
      dmd.drawString(5, 4, "OK");
      break;
  }

  testNum = (testNum + 1) % 9;
}
