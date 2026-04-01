/*
  Basketball Scoreboard — Arduino UNO + P10 LED Matrix
  ======================================================
  Receives game data from PC via USB serial (serial_bridge.py)
  Displays on P10 16×32 panel using DMD2 library.

  DISPLAY CYCLE (each screen = 2.5 seconds):
    1. "UNO CONNECTED" or "WAITING..."  (startup)
    2. Team A name  (e.g. "SRIT")
    3. Team A score (e.g. "16")
    4. Team B name  (e.g. "SREC")
    5. Team B score (e.g. "3")
    6. Clock        (e.g. "11:47" or "45.2")
    7. Quarter      (e.g. "Q 3")
    → repeat

  WIRING (Arduino UNO + P10 HUB12):
  ────────────────────────────────────────────
  Arduino Pin │ Panel Pin │ Label
  ────────────┼───────────┼──────
      13      │  Pin 6    │ CLK
      11      │  Pin 12   │ DATA (R)
       9      │  Pin 8    │ STB  (S)
       6      │  Pin 1    │ OE
       7      │  Pin 2    │ A
       8      │  Pin 4    │ B
      GND     │  Pin 3    │ N (GND)
  ────────────────────────────────────────────
  5V Supply (+) → Panel VCC
  5V Supply (–) → Panel GND + Arduino GND
  Arduino       → USB to PC (serial_bridge.py)

  LIBRARY:
    Sketch → Manage Libraries → search "DMD2" → Install "DMD2 by Freetronics"
*/

#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial_Black_16.h>

// ── Panel config ──────────────────────────────────────────────────────
#define DISPLAYS_WIDE  1
#define DISPLAYS_HIGH  1
SoftDMD dmd(DISPLAYS_WIDE, DISPLAYS_HIGH);

// ── Display timing ────────────────────────────────────────────────────
#define SCREEN_DURATION  2500   // ms each screen shows

// ── Game state ────────────────────────────────────────────────────────
char teamA[8]  = "TEAM A";
char teamB[8]  = "TEAM B";
char scoreA[5] = "0";
char scoreB[5] = "0";
char clock_str[10] = "00:00";
char quarter[4]    = "Q1";
bool dataReceived  = false;

// ── Serial buffer ─────────────────────────────────────────────────────
char serialBuf[64];
int  serialIdx = 0;

// ── Display cycle ─────────────────────────────────────────────────────
int           screen     = 0;
unsigned long lastSwitch = 0;
#define TOTAL_SCREENS  7

// ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  dmd.setBrightness(255);
  dmd.selectFont(SystemFont5x7);
  dmd.begin();

  showCentered("WAIT..");
  Serial.println("Arduino ready. Waiting for serial_bridge.py...");
}

// ─────────────────────────────────────────────────────────────────────
void loop() {
  readSerial();
  cycleDisplay();
}

// ── Read serial data from PC ──────────────────────────────────────────
void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuf[serialIdx] = '\0';
      parsePacket(serialBuf);
      serialIdx = 0;
    } else if (serialIdx < 63) {
      serialBuf[serialIdx++] = c;
    }
  }
}

// ── Parse packet: S,SRIT,16,SREC,3,11:47,3 ───────────────────────────
void parsePacket(const char* buf) {
  if (buf[0] != 'S') return;

  char tmp[64];
  strncpy(tmp, buf + 2, sizeof(tmp));  // skip "S,"

  char* tok = strtok(tmp, ",");
  if (!tok) return; strncpy(teamA, tok, 7);

  tok = strtok(NULL, ",");
  if (!tok) return; strncpy(scoreA, tok, 4);

  tok = strtok(NULL, ",");
  if (!tok) return; strncpy(teamB, tok, 7);

  tok = strtok(NULL, ",");
  if (!tok) return; strncpy(scoreB, tok, 4);

  tok = strtok(NULL, ",");
  if (!tok) return; strncpy(clock_str, tok, 9);

  tok = strtok(NULL, ",");
  if (tok) {
    snprintf(quarter, 4, "Q%s", tok);
  }

  dataReceived = true;
}

// ── Cycle through screens ─────────────────────────────────────────────
void cycleDisplay() {
  if (millis() - lastSwitch < SCREEN_DURATION) return;
  lastSwitch = millis();

  dmd.clearScreen();
  dmd.selectFont(SystemFont5x7);

  if (!dataReceived) {
    // Blink WAIT and CONNECTED alternately
    static bool tog = false;
    showCentered(tog ? "WAIT.." : "READY?");
    tog = !tog;
    return;
  }

  switch (screen) {
    case 0:
      // "UNO" + "LIVE" indicator
      dmd.drawString(0, 0, "UNO");
      dmd.drawString(18, 0, "ON");
      dmd.drawString(0, 9, "LIVE");
      break;

    case 1:
      // Team A label + name
      dmd.drawString(0, 0, "TEAM");
      dmd.drawString(0, 9, teamA);
      break;

    case 2:
      // Team A score — large font
      dmd.selectFont(Arial_Black_16);
      showCenteredLarge(scoreA);
      break;

    case 3:
      // Team B label + name
      dmd.drawString(0, 0, "TEAM");
      dmd.drawString(0, 9, teamB);
      break;

    case 4:
      // Team B score — large font
      dmd.selectFont(Arial_Black_16);
      showCenteredLarge(scoreB);
      break;

    case 5:
      // Clock
      dmd.drawString(0, 0, "TIME");
      dmd.drawString(0, 9, clock_str);
      break;

    case 6:
      // Quarter
      dmd.drawString(0, 0, "PRD");
      dmd.drawString(2, 9, quarter);
      break;
  }

  screen = (screen + 1) % TOTAL_SCREENS;
}

// ── Show small text centred horizontally ─────────────────────────────
void showCentered(const char* msg) {
  dmd.selectFont(SystemFont5x7);
  int w = dmd.stringWidth(msg);
  int x = (32 - w) / 2;
  if (x < 0) x = 0;
  dmd.drawString(x, 4, msg);
}

// ── Show large text centred horizontally ─────────────────────────────
void showCenteredLarge(const char* msg) {
  dmd.selectFont(Arial_Black_16);
  int w = dmd.stringWidth(msg);
  int x = (32 - w) / 2;
  if (x < 0) x = 0;
  dmd.drawString(x, 0, msg);
}
