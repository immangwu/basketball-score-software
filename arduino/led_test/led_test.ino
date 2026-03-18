/*
  LED Matrix Test — NodeMCU + HUB12  16 rows × 32 columns  1/8 scan
  ==================================================================
  No WiFi needed. Upload and watch the panel.
  Serial Monitor at 115200 baud.

  Wiring:
    D1 → OE    D2 → A    D3 → B    D4 → C (Pin 10 / L on board)
    D5 → CLK   D8 → STB  D7 → DATA
    GND → Panel N,  5V supply → Panel VCC,  Supply GND → NodeMCU GND
*/

#define PIN_OE   D1
#define PIN_A    D2
#define PIN_B    D3
#define PIN_C    D4   // Panel pin 10 (L) → D4
#define PIN_CLK  D5
#define PIN_STB  D8
#define PIN_DATA D7

#define ROWS        16
#define COLS        32   // 32 columns
#define BYTES_PER_ROW 4  // 32 cols / 8 bits = 4 bytes
#define SCAN_LINES   8   // 1/8 scan

// Frame buffer: [row][4 bytes for 32 columns]
byte fb[ROWS][BYTES_PER_ROW];
byte scanStep = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_OE,   OUTPUT);
  pinMode(PIN_A,    OUTPUT);
  pinMode(PIN_B,    OUTPUT);
  pinMode(PIN_C,    OUTPUT);
  pinMode(PIN_CLK,  OUTPUT);
  pinMode(PIN_STB,  OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_OE, HIGH);

  Serial.println("\n=== LED Matrix Test  16x32 ===");
}

void loop() {
  Serial.println("TEST 1: All LEDs ON");
  fillAll(0xFF);
  runScan(3000);

  Serial.println("TEST 2: All LEDs OFF");
  fillAll(0x00);
  runScan(1000);

  Serial.println("TEST 3: Checkerboard");
  for (int r = 0; r < ROWS; r++)
    for (int b = 0; b < BYTES_PER_ROW; b++)
      fb[r][b] = (r % 2 == 0) ? 0b10101010 : 0b01010101;
  runScan(3000);

  Serial.println("TEST 4: Rows one by one");
  for (int r = 0; r < ROWS; r++) {
    memset(fb, 0, sizeof(fb));
    for (int b = 0; b < BYTES_PER_ROW; b++) fb[r][b] = 0xFF;
    Serial.printf("  Row %d\n", r);
    runScan(400);
  }

  Serial.println("TEST 5: Columns one by one");
  for (int c = 0; c < COLS; c++) {
    memset(fb, 0, sizeof(fb));
    for (int r = 0; r < ROWS; r++)
      fb[r][c / 8] |= (0x80 >> (c % 8));
    Serial.printf("  Col %d\n", c);
    runScan(200);
  }

  Serial.println("--- Cycle done, repeating ---\n");
}

void runScan(int ms) {
  unsigned long end = millis() + ms;
  while (millis() < end) scanMatrix();
}

void fillAll(byte val) {
  for (int r = 0; r < ROWS; r++)
    for (int b = 0; b < BYTES_PER_ROW; b++)
      fb[r][b] = val;
}

void scanMatrix() {
  static unsigned long t = 0;
  if (micros() - t < 2000) return;
  t = micros();

  digitalWrite(PIN_OE, HIGH);

  // 1/8 scan: step drives row[scanStep] and row[scanStep+8]
  // Shift bottom row first (scanStep+8), then top row (scanStep)
  // Each row = 4 bytes, shifted MSB first, last byte = leftmost cols
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

inline void shiftByte(byte b) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_DATA, (b >> i) & 1);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_CLK, LOW);
  }
}
