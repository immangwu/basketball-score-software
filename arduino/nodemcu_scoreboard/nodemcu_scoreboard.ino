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

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

// ── WiFi & Server ─────────────────────────────────────────────────────
const char* WIFI_SSID   = "HIVE";
const char* WIFI_PASS   = "hive@srit2024";
const char* SERVER_IP   = "172.16.50.85";
const int   SERVER_PORT = 8765;

// ── HUB12 Pins — NodeMCU (matched to board photo labels) ────────────
#define PIN_OE   D1   // GPIO5  → OE  (Output Enable, active LOW)
#define PIN_A    D2   // GPIO4  → A   (Row address bit 0)
#define PIN_B    D3   // GPIO0  → B   (Row address bit 1)
#define PIN_C    D4   // GPIO2  → C   (Row address bit 2) — Panel pin 10 (L) → D4
#define PIN_CLK  D5   // GPIO14 → F   (Clock)
#define PIN_STB  D8   // GPIO15 → S   (Strobe / Latch)
#define PIN_DATA D7   // GPIO13 → R   (Serial data)

// ── Matrix dimensions ─────────────────────────────────────────────────
#define COLS          32   // 32 columns
#define ROWS          16
#define BYTES_PER_ROW  4   // 32 cols / 8 = 4 bytes per row
#define SCAN_LINES     8   // 1/8 scan

// ── Frame buffer: [row][4 bytes for 32 columns] ───────────────────────
// fb[r][0]=cols 0–7, [1]=cols 8–15, [2]=cols 16–23, [3]=cols 24–31
byte fb[ROWS][BYTES_PER_ROW];

// ── 5×7 font for SCORES (big, fills rows 0–6) ────────────────────────
// 7 rows × 5 cols. Bits 7-3 of each byte = 5 pixel columns.
const byte FONT5x7[10][7] = {
  {0b11111000,0b10001000,0b10001000,0b10001000,0b10001000,0b10001000,0b11111000}, // 0
  {0b00100000,0b01100000,0b00100000,0b00100000,0b00100000,0b00100000,0b01110000}, // 1
  {0b11111000,0b00001000,0b00001000,0b11111000,0b10000000,0b10000000,0b11111000}, // 2
  {0b11111000,0b00001000,0b00001000,0b11111000,0b00001000,0b00001000,0b11111000}, // 3
  {0b10001000,0b10001000,0b10001000,0b11111000,0b00001000,0b00001000,0b00001000}, // 4
  {0b11111000,0b10000000,0b10000000,0b11111000,0b00001000,0b00001000,0b11111000}, // 5
  {0b11111000,0b10000000,0b10000000,0b11111000,0b10001000,0b10001000,0b11111000}, // 6
  {0b11111000,0b00001000,0b00010000,0b00100000,0b01000000,0b01000000,0b01000000}, // 7
  {0b11111000,0b10001000,0b10001000,0b11111000,0b10001000,0b10001000,0b11111000}, // 8
  {0b11111000,0b10001000,0b10001000,0b11111000,0b00001000,0b00001000,0b11111000}, // 9
};

// ── 3×5 font for CLOCK (small, rows 9–13) ────────────────────────────
const byte FONT3x5[10][5] = {
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
WiFiClient wifiClient;

// ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_OE,   OUTPUT);
  pinMode(PIN_A,    OUTPUT);
  pinMode(PIN_B,    OUTPUT);
  pinMode(PIN_C,    OUTPUT);
  pinMode(PIN_CLK,  OUTPUT);
  pinMode(PIN_STB,  OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  digitalWrite(PIN_OE, HIGH);
  digitalWrite(PIN_STB, LOW);

  Serial.println("\n========================================");
  Serial.println("  HIVE Basketball Scoreboard — NodeMCU");
  Serial.println("  Sri Ramakrishna Institute of Technology");
  Serial.println("========================================");

  // Boot: fill all LEDs then clear
  for (int r=0; r<ROWS; r++)
    for (int b=0; b<BYTES_PER_ROW; b++) fb[r][b]=0xFF;
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

// ── 1/8 scan matrix refresh — 16×32 panel ────────────────────────────
byte scanStep = 0;

void scanMatrix() {
  static unsigned long t = 0;
  if (micros() - t < 1000) return;   // ~125Hz total (1ms × 8 steps)
  t = micros();

  digitalWrite(PIN_OE, HIGH);

  // 1/8 scan: step drives row[scanStep] (top) and row[scanStep+8] (bottom)
  // Each row = 4 bytes (32 cols). Shift top row first, then bottom.
  for (int b = 0; b < BYTES_PER_ROW; b++)
    shiftByte(fb[scanStep][b]);
  for (int b = 0; b < BYTES_PER_ROW; b++)
    shiftByte(fb[scanStep+8][b]);

  digitalWrite(PIN_STB, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_STB, LOW);

  digitalWrite(PIN_A, (scanStep >> 0) & 1);
  digitalWrite(PIN_B, (scanStep >> 1) & 1);
  digitalWrite(PIN_C, (scanStep >> 2) & 1);

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
    if ((millis()/500) % 2) {
      drawBigDigit(scoreA/10, 0, 2);
      drawBigDigit(scoreA%10, 0, 8);
      drawBigDigit(scoreB/10, 0, 17);
      drawBigDigit(scoreB%10, 0, 23);
    }
    // "END" centre rows 9–13
    drawSmallDigit(3, 9, 12); // borrow digit shape for visual indicator
    for (int c=8; c<24; c++) setPixel(9,c);  // solid bar = END indicator
    return;
  }

  // ── 32×16 Layout ─────────────────────────────────────────────────
  //  Rows 0–6  : Scores  (5×7 big font)
  //    Team A: cols 2–12  (tens at 2, units at 8)
  //    Divider: col 14–15 (vertical line)
  //    Team B: cols 17–27 (tens at 17, units at 23)
  //  Row  7    : blank gap
  //  Rows 8–12 : Clock   (3×5 small font, centred)
  //  Rows 13–15: Quarter (left) + Possession dot (right)

  // ── Scores (5×7 big font) ────────────────────────────────────────
  drawBigDigit(scoreA/10, 0, 2);
  drawBigDigit(scoreA%10, 0, 8);

  // Centre vertical divider
  for (int r=0; r<13; r++) { setPixel(r,14); setPixel(r,15); }

  drawBigDigit(scoreB/10, 0, 17);
  drawBigDigit(scoreB%10, 0, 23);

  // ── Clock (3×5 small font) ────────────────────────────────────────
  if (clockSecs < 60) {
    // SS  centred at cols 13–19
    drawSmallDigit(clockSecs/10, 8, 13);
    drawSmallDigit(clockSecs%10, 8, 17);
    // tenths dots
    for (int d=0; d<clockTen; d++) setPixel(13, 13+d);
  } else {
    int m = clockSecs/60, s = clockSecs%60;
    // MM:SS  cols 5–26
    drawSmallDigit(m/10, 8,  5);
    drawSmallDigit(m%10, 8,  9);
    if ((millis()/500)%2) { setPixel(9,13); setPixel(11,13); }
    drawSmallDigit(s/10, 8, 16);
    drawSmallDigit(s%10, 8, 20);
  }

  // ── Quarter + possession (rows 13–15) ────────────────────────────
  int q = constrain(quarter,1,9);
  // "Q" marker
  setPixel(13,0); setPixel(13,1); setPixel(13,2);
  setPixel(14,0);                 setPixel(14,2);
  setPixel(15,0); setPixel(15,1); setPixel(15,3);
  // Quarter digit (3×5, rows 13–15, col 4)
  for (int r=0; r<3; r++) {
    byte px=(FONT3x5[q][r+1]>>5)&0x07;
    for (int c=0; c<3; c++)
      if (px&(1<<(2-c))) setPixel(13+r, 4+c);
  }
  // Possession dot
  if      (poss=='A') { setPixel(14,28); setPixel(15,28); }
  else if (poss=='B') { setPixel(14,30); setPixel(15,30); }
}

// ── Draw 5×7 big digit (for scores) ──────────────────────────────────
void drawBigDigit(int d, int row, int col) {
  d = constrain(d,0,9);
  for (int r=0; r<7; r++) {
    byte px = (FONT5x7[d][r] >> 3) & 0x1F;  // 5 bits
    for (int c=0; c<5; c++)
      if (px & (1<<(4-c))) setPixel(row+r, col+c);
  }
}

// ── Draw 3×5 small digit (for clock) ─────────────────────────────────
void drawSmallDigit(int d, int row, int col) {
  d = constrain(d,0,9);
  for (int r=0; r<5; r++) {
    byte px = (FONT3x5[d][r] >> 5) & 0x07;
    for (int c=0; c<3; c++)
      if (px & (1<<(2-c))) setPixel(row+r, col+c);
  }
}

// ── Set pixel ─────────────────────────────────────────────────────────
void setPixel(int row, int col) {
  if (row<0||row>=ROWS||col<0||col>=COLS) return;
  fb[row][col/8] |= (0x80 >> (col%8));
}

void clearFB() { memset(fb, 0, sizeof(fb)); }

// ── Fetch state from server ───────────────────────────────────────────
void fetchState() {
  String url = String("http://")+SERVER_IP+":"+SERVER_PORT+"/state";
  HTTPClient http;
  http.begin(wifiClient, url);
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
  http.begin(wifiClient, url);
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
