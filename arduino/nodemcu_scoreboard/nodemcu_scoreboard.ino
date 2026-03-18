/*
  =====================================================================
  Basketball Scoreboard — ESP32 + 16×16 LED Matrix (HUB12, 1/4 scan)
  HIVE · Sri Ramakrishna Institute of Technology
  =====================================================================

  EXACT WIRING from board photo labels:
  ─────────────────────────────────────────────────────────────────────
  HUB12 Board Pin │ Label │ ESP32 Pin │ GPIO
  ────────────────┼───────┼───────────┼──────
  Pin  1 (left 1) │  OE   │   GPIO4   │ GPIO4   ← Output Enable
  Pin  2 (right 1)│   A   │  GPIO16   │ GPIO16  ← Row address bit 0
  Pin  3 (left 2) │   N   │   GND     │ GND
  Pin  4 (right 2)│   B   │  GPIO17   │ GPIO17  ← Row address bit 1
  Pin  5 (left 3) │   N   │   GND     │ GND
  Pin  6 (right 3)│   F   │  GPIO18   │ GPIO18  ← CLK (VSPI CLK)
  Pin  7 (left 4) │   N   │   GND     │ GND
  Pin  8 (right 4)│   S   │   GPIO5   │ GPIO5   ← STB/LAT
  Pin  9 (left 5) │   N   │   GND     │ GND
  Pin 10 (right 5)│   L   │  (skip)   │ —
  Pin 11 (left 6) │   N   │   GND     │ GND
  Pin 12 (right 6)│   R   │  GPIO23   │ GPIO23  ← DATA (VSPI MOSI)
  Pin 13 (left 7) │   N   │   GND     │ GND
  Pin 14 (right 7)│   F   │  (skip)   │ —
  Pin 15 (left 8) │   N   │   GND     │ GND
  Pin 16 (right 8)│   N   │   GND     │ GND
  ─────────────────────────────────────────────────────────────────────
  POWER:
    LED Panel VCC (5V rail on board) → External 5V 3A power supply +
    LED Panel GND                    → External 5V supply GND
    ESP32 GND                        → Same external 5V supply GND
    (COMMON GROUND between ESP32 and panel supply is REQUIRED)

  ESP32 powered separately via USB.
  ─────────────────────────────────────────────────────────────────────
  ARDUINO IDE BOARD SETUP:
    Board  : ESP32 Dev Module
    Port   : COMx (whichever appears when ESP32 is plugged in)
    Add boards URL in File → Preferences:
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

  SCAN MODE: 1/4 scan (confirmed: only A and B address pins on board)
    Step 0 (A=0,B=0): rows  0, 4,  8, 12 active together
    Step 1 (A=1,B=0): rows  1, 5,  9, 13 active together
    Step 2 (A=0,B=1): rows  2, 6, 10, 14 active together
    Step 3 (A=1,B=1): rows  3, 7, 11, 15 active together

  Each step: shift 8 bytes (4 rows × 2 bytes for 16 columns) then latch.
  ─────────────────────────────────────────────────────────────────────
  DISPLAY LAYOUT (16×16 pixels):
    Rows  0–5 : Scores  │ Cols 0–7: Team A │ Cols 8–15: Team B
    Row   6   : Separator
    Rows  7–11: Clock (SS.t  or  MM:SS)
    Rows 12–14: Quarter (Q1–Q4)
    Row  15   : Possession dot (left=TeamA, right=TeamB)
  ─────────────────────────────────────────────────────────────────────
  LIBRARY: ArduinoJson v6.x (Sketch → Manage Libraries → ArduinoJson)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ── WiFi & Server ─────────────────────────────────────────────────────
const char* WIFI_SSID   = "HIVE";
const char* WIFI_PASS   = "hive@srit2024";
const char* SERVER_IP   = "172.16.50.85";
const int   SERVER_PORT = 8765;

// ── HUB12 Pins — ESP32 GPIO numbers (matched to board photo labels) ──
#define PIN_OE    4   // GPIO4  → OE  (Output Enable, active LOW)
#define PIN_A    16   // GPIO16 → A   (Row address bit 0)
#define PIN_B    17   // GPIO17 → B   (Row address bit 1)
#define PIN_CLK  18   // GPIO18 → F   (Clock — VSPI CLK)
#define PIN_STB   5   // GPIO5  → S   (Strobe / Latch)
#define PIN_DATA 23   // GPIO23 → R   (Serial data — VSPI MOSI)

// ── Matrix dimensions ─────────────────────────────────────────────────
#define COLS        16
#define ROWS        16
#define SCAN_LINES   4   // 1/4 scan

// ── Frame buffer [row][col_byte] — 2 bytes × 16 rows ─────────────────
// fb[r][0] = columns 0–7,  fb[r][1] = columns 8–15
// Bit 7 of [0] = col 0 (leftmost),  Bit 0 of [1] = col 15 (rightmost)
byte fb[ROWS][2];

// ── 3×5 pixel font (digits 0–9) ──────────────────────────────────────
// 5 bytes per digit, bits 7-5 = 3 pixel columns
const byte FONT[10][5] = {
  {0b11100000,0b10100000,0b10100000,0b10100000,0b11100000}, // 0
  {0b01000000,0b11000000,0b01000000,0b01000000,0b11100000}, // 1
  {0b11100000,0b00100000,0b11100000,0b10000000,0b11100000}, // 2
  {0b11100000,0b00100000,0b11100000,0b00100000,0b11100000}, // 3
  {0b10100000,0b10100000,0b11100000,0b00100000,0b00100000}, // 4
  {0b11100000,0b10000000,0b11100000,0b00100000,0b11100000}, // 5
  {0b11100000,0b10000000,0b11100000,0b10100000,0b11100000}, // 6
  {0b11100000,0b00100000,0b01000000,0b01000000,0b01000000}, // 7
  {0b11100000,0b10100000,0b11100000,0b10100000,0b11100000}, // 8
  {0b11100000,0b10100000,0b11100000,0b00100000,0b11100000}, // 9
};

// ── Game state ────────────────────────────────────────────────────────
int  scoreA=0, scoreB=0, clockSecs=0, clockTen=0, quarter=1;
bool running=false, gameOver=false;
char poss='N';

// ── Timing ────────────────────────────────────────────────────────────
unsigned long lastFetch=0, lastPing=0;

// ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_OE,   OUTPUT);
  pinMode(PIN_A,    OUTPUT);
  pinMode(PIN_B,    OUTPUT);
  pinMode(PIN_CLK,  OUTPUT);
  pinMode(PIN_STB,  OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  digitalWrite(PIN_OE, HIGH);    // disable until ready
  digitalWrite(PIN_STB, LOW);

  Serial.println("\n========================================");
  Serial.println("  HIVE Basketball Scoreboard — NodeMCU");
  Serial.println("  Sri Ramakrishna Institute of Technology");
  Serial.println("========================================");

  // Boot: fill display then clear
  for (int r=0; r<ROWS; r++) { fb[r][0]=0xFF; fb[r][1]=0xFF; }
  for (int i=0; i<200; i++) { scanMatrix(); delay(1); }
  clearFB();

  connectWiFi();
}

// ─────────────────────────────────────────────────────────────────────
void loop() {
  scanMatrix();   // keep calling — handles one row per call

  if (WiFi.status() != WL_CONNECTED) { connectWiFi(); return; }

  unsigned long now = millis();
  if (now - lastFetch >= 500)  { lastFetch=now; fetchState(); renderBoard(); }
  if (now - lastPing  >= 2000) { lastPing=now;  sendPing(); }
}

// ── 1/4 scan matrix refresh ───────────────────────────────────────────
// Each call handles one scan step (4 rows at a time)
byte scanStep = 0;

void scanMatrix() {
  static unsigned long t = 0;
  if (micros() - t < 2000) return;   // ~500Hz total (2ms per step × 4 steps)
  t = micros();

  // Disable output while shifting
  digitalWrite(PIN_OE, HIGH);

  // For 1/4 scan: step drives rows: scanStep, scanStep+4, scanStep+8, scanStep+12
  // Shift order (last shifted = first in chain = closest to output):
  // row+12 high, row+12 low, row+8 high, row+8 low, row+4 high, row+4 low, row high, row low
  shiftByte(fb[scanStep+12][1]);
  shiftByte(fb[scanStep+12][0]);
  shiftByte(fb[scanStep+ 8][1]);
  shiftByte(fb[scanStep+ 8][0]);
  shiftByte(fb[scanStep+ 4][1]);
  shiftByte(fb[scanStep+ 4][0]);
  shiftByte(fb[scanStep   ][1]);
  shiftByte(fb[scanStep   ][0]);

  // Strobe/latch pulse
  digitalWrite(PIN_STB, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_STB, LOW);

  // Set row address (A = bit0, B = bit1)
  digitalWrite(PIN_A, scanStep & 1);
  digitalWrite(PIN_B, (scanStep >> 1) & 1);

  // Enable output
  digitalWrite(PIN_OE, LOW);

  scanStep = (scanStep + 1) % SCAN_LINES;
}

// ── Shift one byte, MSB first ─────────────────────────────────────────
inline void shiftByte(byte b) {
  for (int i=7; i>=0; i--) {
    digitalWrite(PIN_DATA, (b >> i) & 1);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_CLK, LOW);
  }
}

// ── Render scoreboard into frame buffer ──────────────────────────────
void renderBoard() {
  clearFB();

  if (gameOver) {
    // Blink scores
    if ((millis()/500) % 2) {
      drawDigit(scoreA/10, 0, 0);
      drawDigit(scoreA%10, 0, 4);
      drawDigit(scoreB/10, 0, 8);
      drawDigit(scoreB%10, 0, 12);
    }
    // "FIN" rows 7-13
    drawLetter('F', 7, 0);
    drawLetter('I', 7, 5);
    drawLetter('N', 7, 10);
    return;
  }

  // ── Rows 0–5: Scores ─────────────────────────────────────────────
  // Team A (cols 0–7): tens at col 0, units at col 4
  drawDigit(scoreA/10, 0, 0);
  drawDigit(scoreA%10, 0, 4);
  // Team B (cols 8–15): tens at col 8, units at col 12
  drawDigit(scoreB/10, 0, 8);
  drawDigit(scoreB%10, 0, 12);

  // ── Row 6: Centre separator ───────────────────────────────────────
  fb[6][0] = 0b10001000;
  fb[6][1] = 0b10001000;

  // ── Rows 7–11: Clock ──────────────────────────────────────────────
  if (clockSecs < 60) {
    // SS.t — seconds 2 digits, tenths bar
    drawDigit(clockSecs/10, 7, 1);
    drawDigit(clockSecs%10, 7, 6);
    // Tenths: dots on row 12 (0–9 pixels from left)
    for (int d=0; d<clockTen; d++) setPixel(12, d+1);
  } else {
    // MM:SS — minutes single digit + colon + seconds 2 digits
    int m = clockSecs/60, s = clockSecs%60;
    drawDigit(m%10, 7, 0);
    // colon blink
    if ((millis()/500)%2) { setPixel(8,4); setPixel(10,4); }
    drawDigit(s/10, 7, 6);
    drawDigit(s%10, 7, 11);
  }

  // ── Rows 13–15: Quarter ───────────────────────────────────────────
  // "Q" 3×3 then quarter digit
  // Q shape
  setPixel(13,0); setPixel(13,1);
  setPixel(14,0);               setPixel(14,2);
  setPixel(15,0); setPixel(15,1); setPixel(15,2);
  // Quarter number (3×5 font, small — use rows 13–15 only, 3 rows)
  int q = constrain(quarter,1,9);
  byte qrows[3] = { FONT[q][1], FONT[q][2], FONT[q][3] }; // middle 3 rows of digit
  for (int r=0; r<3; r++) {
    byte px = (qrows[r] >> 5) & 0x07;
    for (int c=0; c<3; c++) {
      if (px & (1<<(2-c))) setPixel(13+r, 4+c);
    }
  }

  // ── Row 15: Possession dot ────────────────────────────────────────
  if      (poss=='A') { setPixel(15,8);  setPixel(15,9);  }
  else if (poss=='B') { setPixel(15,13); setPixel(15,14); }
}

// ── Draw 3×5 digit at (row, col) ──────────────────────────────────────
void drawDigit(int d, int row, int col) {
  d = constrain(d,0,9);
  for (int r=0; r<5; r++) {
    byte px = (FONT[d][r] >> 5) & 0x07;
    for (int c=0; c<3; c++)
      if (px & (1<<(2-c))) setPixel(row+r, col+c);
  }
}

// ── Simple 3-wide capital letters for FIN ─────────────────────────────
const byte LETTER_F[5] = {0b11100000,0b10000000,0b11000000,0b10000000,0b10000000};
const byte LETTER_I[5] = {0b11100000,0b01000000,0b01000000,0b01000000,0b11100000};
const byte LETTER_N[5] = {0b10100000,0b11100000,0b11100000,0b10100000,0b10100000};

void drawLetter(char ch, int row, int col) {
  const byte* src = nullptr;
  if (ch=='F') src=LETTER_F;
  else if (ch=='I') src=LETTER_I;
  else if (ch=='N') src=LETTER_N;
  if (!src) return;
  for (int r=0; r<5; r++) {
    byte px=(src[r]>>5)&0x07;
    for (int c=0; c<3; c++)
      if (px&(1<<(2-c))) setPixel(row+r,col+c);
  }
}

// ── Set pixel ─────────────────────────────────────────────────────────
void setPixel(int row, int col) {
  if (row<0||row>=ROWS||col<0||col>=COLS) return;
  fb[row][col/8] |= (0x80 >> (col%8));
}

void clearFB() { memset(fb,0,sizeof(fb)); }

// ── Fetch state from server ───────────────────────────────────────────
void fetchState() {
  String url = String("http://")+SERVER_IP+":"+SERVER_PORT+"/state";
  HTTPClient http;
  http.begin(url);
  http.setTimeout(800);
  if (http.GET() != 200) { http.end(); return; }
  String body = http.getString();
  http.end();

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc,body)) return;

  scoreA    = doc["sa"]|0;
  scoreB    = doc["sb"]|0;
  clockSecs = doc["ci"]|0;
  clockTen  = doc["ct"]|0;
  quarter   = doc["q"] |1;
  running   = (doc["run"]==1);
  gameOver  = (doc["go"] ==1);
  poss      = ((const char*)doc["pos"])[0];

  // Serial Monitor output
  char ck[10];
  if (clockSecs>=60) sprintf(ck,"%02d:%02d",clockSecs/60,clockSecs%60);
  else               sprintf(ck,"%02d.%d",clockSecs,clockTen);
  Serial.printf("%-6s %2d | %2d %-6s  %s  Q%d  %s\n",
    (const char*)doc["na"], scoreA, scoreB, (const char*)doc["nb"],
    ck, quarter, running?"RUN":"PAUSE");
}

// ── Heartbeat ─────────────────────────────────────────────────────────
void sendPing() {
  String url = String("http://")+SERVER_IP+":"+SERVER_PORT+"/ping";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  http.POST("{}");
  http.end();
}

// ── WiFi ──────────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries=0;
  while (WiFi.status()!=WL_CONNECTED && tries<30) {
    scanMatrix(); delay(500);
    Serial.print("."); tries++;
  }
  if (WiFi.status()==WL_CONNECTED)
    Serial.printf("\nConnected! IP: %s\n\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\nFailed — retrying...");
}
