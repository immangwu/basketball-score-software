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
  // Each row = 4 bytes (32 cols). Shift bottom row first, then top.
  for (int b = BYTES_PER_ROW-1; b >= 0; b--)
    shiftByte(fb[scanStep+8][b]);
  for (int b = BYTES_PER_ROW-1; b >= 0; b--)
    shiftByte(fb[scanStep][b]);

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

  // ── 32×16 Layout ─────────────────────────────────────────────────
  //  Cols  0–13 : Team A score (large 5×5 digits, centred in left half)
  //  Col  14–17 : Centre divider + clock colon
  //  Cols 18–31 : Team B score (large 5×5 digits, centred in right half)
  //  Rows  0– 5 : Scores
  //  Row   6    : Separator line
  //  Rows  7–11 : Clock (MM:SS or SS.t)
  //  Rows 12–15 : Quarter + possession

  // Team A score — big digits at cols 0,4 (tens,units) rows 0–5
  drawDigit(scoreA/10, 0, 1);
  drawDigit(scoreA%10, 0, 6);

  // Vertical divider cols 12–13
  for (int r=0; r<12; r++) { setPixel(r,12); }

  // Team B score — big digits at cols 18,23 rows 0–5
  drawDigit(scoreB/10, 0, 18);
  drawDigit(scoreB%10, 0, 23);

  // ── Row 6: Horizontal separator ──────────────────────────────────
  for (int c=0; c<COLS; c++) setPixel(6,c);

  // ── Rows 7–11: Clock centred ─────────────────────────────────────
  if (clockSecs < 60) {
    // SS.t — 2 digits centred (cols 10–17)
    drawDigit(clockSecs/10, 7, 11);
    drawDigit(clockSecs%10, 7, 16);
    // tenths bar on row 12
    for (int d=0; d<clockTen; d++) setPixel(12, 12+d);
  } else {
    int m = clockSecs/60, s = clockSecs%60;
    // MM:SS — 4 digits + colon (cols 7–24)
    drawDigit(m/10, 7,  7);
    drawDigit(m%10, 7, 12);
    if ((millis()/500)%2) { setPixel(8,17); setPixel(10,17); }
    drawDigit(s/10, 7, 18);
    drawDigit(s%10, 7, 23);
  }

  // ── Rows 12–15: Quarter label (left) + possession dot (right) ────
  // "Q" at col 0
  setPixel(13,0); setPixel(13,1); setPixel(13,2);
  setPixel(14,0);                 setPixel(14,2);
  setPixel(15,0); setPixel(15,1); setPixel(15,3);
  // Quarter digit
  int q = constrain(quarter,1,9);
  for (int r=0; r<5; r++) {
    byte px=(FONT[q][r]>>5)&0x07;
    for (int c=0; c<3; c++)
      if (px&(1<<(2-c))) setPixel(11+r, 5+c);
  }
  // Possession: left dot = A, right dot = B
  if      (poss=='A') { setPixel(14,27); setPixel(15,27); }
  else if (poss=='B') { setPixel(14,30); setPixel(15,30); }
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
