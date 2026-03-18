/*
  =====================================================================
  Basketball Scoreboard — NodeMCU + 16×16 LED Matrix (HUB12 / P10)
  HIVE · Sri Ramakrishna Institute of Technology
  =====================================================================

  HARDWARE: 16×16 single-color LED panel with 16-pin HUB12 connector
            Internal driver uses 74HC595 shift registers
            Scan mode: 1/8 scan (most common for 16-row panels)

  ─────────────────────────────────────────────────────────────────────
  WIRING — NodeMCU → HUB12 16-pin IDC connector
  ─────────────────────────────────────────────────────────────────────
  NodeMCU Pin │ GPIO  │ HUB12 Pin │ Signal  │ Description
  ────────────┼───────┼───────────┼─────────┼──────────────────────
  D1          │ GPIO5 │  Pin  7   │  /OE    │ Output Enable (active LOW)
  D2          │ GPIO4 │  Pin  2   │   A     │ Row address bit 0
  D3          │ GPIO0 │  Pin  3   │   B     │ Row address bit 1
  D4          │ GPIO2 │  Pin  9   │   C     │ Row address bit 2 (1/8 scan)
  D5          │GPIO14 │  Pin  6   │  CLK    │ Shift register clock
  D7          │GPIO13 │  Pin 12   │   R     │ Serial data (column bits)
  D8          │GPIO15 │  Pin 15   │  LAT    │ Latch (store data to output)
  GND         │  GND  │  1,4,5,8, │  GND    │ All GND pins together
              │       │  11,13,14,│         │
              │       │  16       │         │
  ─────────────────────────────────────────────────────────────────────
  IMPORTANT: LED panel needs its OWN 5V 3A power supply.
             Connect panel GND to NodeMCU GND (common ground).
             NodeMCU 3.3V signals work with most 5V HUB12 panels.
             If display flickers, add 74HCT245 level shifter.

  HUB12 16-pin layout (IDC connector, viewed from front):
    1  GND    2  A
    3  B      4  GND
    5  GND    6  CLK
    7  /OE    8  GND
    9  C     10  GND
   11  GND   12  R (data)
   13  GND   14  GND
   15  LAT   16  GND

  ─────────────────────────────────────────────────────────────────────
  DISPLAY LAYOUT (16×16 pixels):
  ─────────────────────────────────────────────────────────────────────
  Rows  0-5 : Score row  │ Cols 0-7: Team A │ Cols 8-15: Team B
  Row   6   : Separator line
  Rows  7-11: Clock (seconds, 2 digits centered)
  Rows 12-13: Quarter label (Q1–Q4)
  Rows 14-15: Possession indicator (A=left dot, B=right dot)

  ─────────────────────────────────────────────────────────────────────
  LIBRARIES NEEDED (install via Sketch → Manage Libraries):
    • ArduinoJson  v6.x  by Benoit Blanchon
  ─────────────────────────────────────────────────────────────────────
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

// ── WiFi & Server settings — CHANGE THESE ────────────────────────────
const char* WIFI_SSID   = "HIVE";
const char* WIFI_PASS   = "hive@srit2024";
const char* SERVER_IP   = "172.16.50.85";
const int   SERVER_PORT = 8765;

// ── HUB12 Pin Definitions ─────────────────────────────────────────────
#define PIN_OE   D1   // GPIO5  — Output Enable (active LOW)
#define PIN_A    D2   // GPIO4  — Row address bit 0
#define PIN_B    D3   // GPIO0  — Row address bit 1
#define PIN_C    D4   // GPIO2  — Row address bit 2
#define PIN_CLK  D5   // GPIO14 — Clock
#define PIN_DATA D7   // GPIO13 — Serial data
#define PIN_LAT  D8   // GPIO15 — Latch

// ── Display size ──────────────────────────────────────────────────────
#define COLS       16
#define ROWS       16
#define SCAN_LINES  8   // 1/8 scan → 8 address steps, rows paired (0+8,1+9...)

// ── Frame buffer: [row][2 bytes for 16 columns] ───────────────────────
// Bit 7 of byte 0 = column 0 (leftmost)
// Bit 0 of byte 1 = column 15 (rightmost)
byte fb[ROWS][2];

// ── 3×5 Pixel Font (digits 0–9) ───────────────────────────────────────
// 5 rows, each row = 3 bits, stored in bits 7-5 of each byte
const byte FONT[10][5] = {
  {0b11100000, 0b10100000, 0b10100000, 0b10100000, 0b11100000}, // 0
  {0b01000000, 0b11000000, 0b01000000, 0b01000000, 0b11100000}, // 1
  {0b11100000, 0b00100000, 0b11100000, 0b10000000, 0b11100000}, // 2
  {0b11100000, 0b00100000, 0b11100000, 0b00100000, 0b11100000}, // 3
  {0b10100000, 0b10100000, 0b11100000, 0b00100000, 0b00100000}, // 4
  {0b11100000, 0b10000000, 0b11100000, 0b00100000, 0b11100000}, // 5
  {0b11100000, 0b10000000, 0b11100000, 0b10100000, 0b11100000}, // 6
  {0b11100000, 0b00100000, 0b01000000, 0b01000000, 0b01000000}, // 7
  {0b11100000, 0b10100000, 0b11100000, 0b10100000, 0b11100000}, // 8
  {0b11100000, 0b10100000, 0b11100000, 0b00100000, 0b11100000}, // 9
};

// ── 3×3 Mini font for quarter digit ──────────────────────────────────
const byte MINI[10][3] = {
  {0b11100000, 0b10100000, 0b11100000}, // 0
  {0b11000000, 0b01000000, 0b11100000}, // 1
  {0b11100000, 0b11100000, 0b11100000}, // 2
  {0b11100000, 0b11100000, 0b11100000}, // 3
  {0b10100000, 0b11100000, 0b00100000}, // 4
  {0b11100000, 0b11000000, 0b11100000}, // 5
  {0b10000000, 0b11100000, 0b11100000}, // 6
  {0b11100000, 0b00100000, 0b01000000}, // 7
  {0b11100000, 0b11100000, 0b11100000}, // 8
  {0b11100000, 0b11100000, 0b00100000}, // 9
};

// ── Game state ────────────────────────────────────────────────────────
int  scoreA    = 0;
int  scoreB    = 0;
int  clockSecs = 0;
int  clockTen  = 0;
int  quarter   = 1;
bool running   = false;
bool gameOver  = false;
char poss      = 'N';

// ── Timing ────────────────────────────────────────────────────────────
unsigned long lastFetch = 0;
unsigned long lastPing  = 0;
const int FETCH_INTERVAL = 500;
const int PING_INTERVAL  = 2000;

WiFiClient wifiClient;

// ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  // Init matrix pins
  pinMode(PIN_OE,   OUTPUT);
  pinMode(PIN_A,    OUTPUT);
  pinMode(PIN_B,    OUTPUT);
  pinMode(PIN_C,    OUTPUT);
  pinMode(PIN_CLK,  OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_LAT,  OUTPUT);

  digitalWrite(PIN_OE, HIGH);   // disable output initially

  clearDisplay();
  showBoot();                   // show startup pattern while connecting

  Serial.println("\n========================================");
  Serial.println("  HIVE Basketball Scoreboard — NodeMCU");
  Serial.println("  Sri Ramakrishna Institute of Technology");
  Serial.println("========================================\n");

  connectWiFi();
}

// ─────────────────────────────────────────────────────────────────────
void loop() {
  scanMatrix();   // continuously refresh display (call as fast as possible)

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }

  unsigned long now = millis();

  if (now - lastFetch >= FETCH_INTERVAL) {
    lastFetch = now;
    fetchState();
    renderScoreboard();
  }

  if (now - lastPing >= PING_INTERVAL) {
    lastPing = now;
    sendPing();
  }
}

// ── Matrix scan (call in loop — 1 row per call, 1ms per row) ─────────
byte scanRow = 0;

void scanMatrix() {
  static unsigned long lastScan = 0;
  if (micros() - lastScan < 1250) return;   // ~800Hz total refresh
  lastScan = micros();

  // Disable output while shifting
  digitalWrite(PIN_OE, HIGH);

  // For 1/8 scan: each address drives row[scanRow] and row[scanRow+8]
  // Shift out 16 bits: top row data (row scanRow), then bottom (row scanRow+8)
  // MSB first: column 0 is bit 7 of fb[row][0]

  // Shift byte 1 of bottom row first (col 8–15)
  shiftByte(fb[scanRow + 8][1]);
  // Shift byte 0 of bottom row (col 0–7)
  shiftByte(fb[scanRow + 8][0]);
  // Shift byte 1 of top row (col 8–15)
  shiftByte(fb[scanRow][1]);
  // Shift byte 0 of top row (col 0–7)
  shiftByte(fb[scanRow][0]);

  // Latch
  digitalWrite(PIN_LAT, HIGH);
  digitalWrite(PIN_LAT, LOW);

  // Set row address
  digitalWrite(PIN_A, (scanRow >> 0) & 1);
  digitalWrite(PIN_B, (scanRow >> 1) & 1);
  digitalWrite(PIN_C, (scanRow >> 2) & 1);

  // Enable output
  digitalWrite(PIN_OE, LOW);

  scanRow = (scanRow + 1) % SCAN_LINES;
}

// ── Shift one byte MSB-first via bit-bang ─────────────────────────────
void shiftByte(byte b) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_DATA, (b >> i) & 1);
    digitalWrite(PIN_CLK, HIGH);
    digitalWrite(PIN_CLK, LOW);
  }
}

// ── Render scoreboard to frame buffer ─────────────────────────────────
void renderScoreboard() {
  clearDisplay();

  if (gameOver) {
    renderGameOver();
    return;
  }

  // ── Row 0-5: Scores ───────────────────────────────────────────────
  // Team A: 2 digits in cols 0-7 (tens at col 0, units at col 4)
  drawDigit(scoreA / 10, 0, 0);   // tens  at col 0, row 0
  drawDigit(scoreA % 10, 0, 4);   // units at col 4, row 0

  // Team B: 2 digits in cols 8-15 (tens at col 8, units at col 12)
  drawDigit(scoreB / 10, 0, 8);
  drawDigit(scoreB % 10, 0, 12);

  // ── Row 6: Separator ──────────────────────────────────────────────
  fb[6][0] = 0b10000001;
  fb[6][1] = 0b10000001;

  // ── Row 7-11: Clock ───────────────────────────────────────────────
  if (clockSecs < 60) {
    // Show SS.t: seconds (2 digits) + tenths dot
    drawDigit(clockSecs / 10, 7, 1);
    drawDigit(clockSecs % 10, 7, 6);
    // Tenths as a single pixel dot (col 11, row 11)
    if (clockTen > 0) setPixel(11, 11 + clockTen / 2);
  } else {
    // Show MM:SS — minutes left digit, seconds right digit (tight fit)
    int m = clockSecs / 60;
    int s = clockSecs % 60;
    drawDigit(m % 10, 7, 0);     // minutes units (only 1 digit if < 10min)
    // Colon blink at col 4
    if ((millis() / 500) % 2) { setPixel(8, 4); setPixel(10, 4); }
    drawDigit(s / 10, 7, 6);
    drawDigit(s % 10, 7, 11);
  }

  // ── Row 12-14: Quarter ────────────────────────────────────────────
  // "Q" represented as 2 pixels, then quarter digit
  fb[12][0] |= 0b01100000;  // Q top
  fb[13][0] |= 0b01100000;  // Q mid
  fb[14][0] |= 0b01110000;  // Q bot
  int q = constrain(quarter, 1, 9);
  // Mini digit at col 3
  for (int r = 0; r < 3; r++) {
    byte px = (MINI[q][r] >> 5) & 0x07;   // 3 bits
    for (int c = 0; c < 3; c++) {
      if (px & (1 << (2 - c))) setPixel(12 + r, 3 + c);
    }
  }

  // ── Row 15: Possession ────────────────────────────────────────────
  if (poss == 'A') {
    fb[15][0] |= 0b11000000;   // 2 dots on left = Team A has ball
  } else if (poss == 'B') {
    fb[15][1] |= 0b00000110;   // 2 dots on right = Team B has ball
  }
}

// ── Draw a 3×5 digit at (startRow, startCol) ─────────────────────────
void drawDigit(int digit, int startRow, int startCol) {
  digit = constrain(digit, 0, 9);
  for (int r = 0; r < 5; r++) {
    byte row = (FONT[digit][r] >> 5) & 0x07;   // 3 bits
    for (int c = 0; c < 3; c++) {
      if (row & (1 << (2 - c))) {
        setPixel(startRow + r, startCol + c);
      }
    }
  }
}

// ── Set a single pixel in frame buffer ───────────────────────────────
void setPixel(int row, int col) {
  if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;
  fb[row][col / 8] |= (0x80 >> (col % 8));
}

// ── Clear frame buffer ────────────────────────────────────────────────
void clearDisplay() {
  memset(fb, 0, sizeof(fb));
}

// ── Game Over display ─────────────────────────────────────────────────
void renderGameOver() {
  // Show winning score blinking
  static bool blink = false;
  blink = !blink;
  if (blink) {
    drawDigit(scoreA / 10, 0, 0);
    drawDigit(scoreA % 10, 0, 4);
    drawDigit(scoreB / 10, 0, 8);
    drawDigit(scoreB % 10, 0, 12);
  }
  // "END" in rows 8-14
  // E
  fb[8][0]  |= 0b11100000;
  fb[9][0]  |= 0b10000000;
  fb[10][0] |= 0b11000000;
  fb[11][0] |= 0b10000000;
  fb[12][0] |= 0b11100000;
  // N
  fb[8][0]  |= 0b00011100;
  fb[9][0]  |= 0b00011100;
  fb[10][0] |= 0b00010100;
  fb[11][0] |= 0b00010100;
  fb[12][0] |= 0b00010100;
  // D
  fb[8][0]  |= 0b00000011; fb[8][1]  |= 0b00000000;
  fb[9][1]  |= 0b10000000;
  fb[10][1] |= 0b10000000;
  fb[11][1] |= 0b10000000;
  fb[12][0] |= 0b00000011;
}

// ── Boot animation (fill rows one by one) ─────────────────────────────
void showBoot() {
  for (int r = 0; r < ROWS; r++) {
    fb[r][0] = 0xFF;
    fb[r][1] = 0xFF;
    for (int i = 0; i < 20; i++) { scanMatrix(); delay(1); }
  }
  delay(300);
  clearDisplay();
}

// ── Fetch game state from server ──────────────────────────────────────
void fetchState() {
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/state";
  HTTPClient http;
  http.begin(wifiClient, url);
  http.setTimeout(800);
  int code = http.GET();
  if (code != 200) {
    http.end();
    return;
  }
  String payload = http.getString();
  http.end();

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, payload)) return;

  scoreA    = doc["sa"] | 0;
  scoreB    = doc["sb"] | 0;
  clockSecs = doc["ci"] | 0;
  clockTen  = doc["ct"] | 0;
  quarter   = doc["q"]  | 1;
  running   = doc["run"] == 1;
  gameOver  = doc["go"]  == 1;
  const char* p = doc["pos"] | "N";
  poss = p[0];

  // ── Also print to Serial Monitor ─────────────────────────────────
  char clkStr[10];
  if (clockSecs >= 60) sprintf(clkStr, "%02d:%02d", clockSecs/60, clockSecs%60);
  else                 sprintf(clkStr, "%02d.%d",   clockSecs, clockTen);

  Serial.printf("%-6s %2d | %2d %-6s  %s  Q%d  %s\n",
    (const char*)doc["na"], scoreA, scoreB, (const char*)doc["nb"],
    clkStr, quarter, running ? "RUN" : "PAUSE");
}

// ── Send heartbeat ────────────────────────────────────────────────────
void sendPing() {
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/ping";
  HTTPClient http;
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  http.POST("{}");
  http.end();
}

// ── WiFi connect ──────────────────────────────────────────────────────
void connectWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 30) {
    delay(500); Serial.print("."); t++;
    scanMatrix();   // keep refreshing display while connecting
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected! IP: %s\n\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nFailed — retrying...");
  }
}
