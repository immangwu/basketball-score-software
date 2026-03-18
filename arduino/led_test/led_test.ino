/*
  Number Display Test — NodeMCU + 16×32 LED Matrix
  =================================================
  Shows numbers 1 to 10 one by one, each for 2 seconds.
  Tests two font sizes — watch which one looks cleaner.

  No WiFi needed. Just upload and observe.
  Serial Monitor at 115200 baud.
*/

#define PIN_OE   D0
#define PIN_A    D1
#define PIN_B    D2
#define PIN_C    D3
#define PIN_CLK  D4
#define PIN_STB  D5
#define PIN_DATA D6

#define ROWS          16
#define COLS          32
#define BYTES_PER_ROW  4
#define SCAN_LINES     8

byte fb[ROWS][BYTES_PER_ROW];
byte scanStep = 0;

// ── 5×7 font (large) ──────────────────────────────────────────────────
const byte F5[10][7] = {
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

// ── 3×5 font (small) ──────────────────────────────────────────────────
const byte F3[10][5] = {
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
  Serial.println("Number test starting...");
}

void loop() {
  // Show 1 to 10 with LARGE font (5×7), centred
  Serial.println("--- LARGE font (5x7) ---");
  for (int n = 1; n <= 10; n++) {
    clearFB();
    if (n < 10) {
      // Single digit — draw centred (col 13)
      drawLarge(n, 4, 13);
    } else {
      // "10" — two digits side by side (cols 9 and 15)
      drawLarge(1, 4, 9);
      drawLarge(0, 4, 16);
    }
    Serial.printf("Large: %d\n", n);
    runScan(2000);
  }

  // Show 1 to 10 with SMALL font (3×5), centred
  Serial.println("--- SMALL font (3x5) ---");
  for (int n = 1; n <= 10; n++) {
    clearFB();
    if (n < 10) {
      // Single digit centred (col 14)
      drawSmall(n, 5, 14);
    } else {
      // "10" — two digits (cols 12 and 16)
      drawSmall(1, 5, 12);
      drawSmall(0, 5, 17);
    }
    Serial.printf("Small: %d\n", n);
    runScan(2000);
  }

  Serial.println("--- Cycle done ---\n");
}

// Draw 5×7 digit at (row, col)
void drawLarge(int d, int row, int col) {
  d = constrain(d, 0, 9);
  for (int r = 0; r < 7; r++) {
    byte px = (F5[d][r] >> 3) & 0x1F;
    for (int c = 0; c < 5; c++)
      if (px & (1 << (4-c))) setPixel(row+r, col+c);
  }
}

// Draw 3×5 digit at (row, col)
void drawSmall(int d, int row, int col) {
  d = constrain(d, 0, 9);
  for (int r = 0; r < 5; r++) {
    byte px = (F3[d][r] >> 5) & 0x07;
    for (int c = 0; c < 3; c++)
      if (px & (1 << (2-c))) setPixel(row+r, col+c);
  }
}

void setPixel(int row, int col) {
  if (row<0||row>=ROWS||col<0||col>=COLS) return;
  fb[row][col/8] |= (0x80 >> (col%8));
}

void clearFB() { memset(fb, 0, sizeof(fb)); }

void runScan(int ms) {
  unsigned long end = millis() + ms;
  while (millis() < end) scanMatrix();
}

void scanMatrix() {
  static unsigned long t = 0;
  if (micros() - t < 1000) return;
  t = micros();

  digitalWrite(PIN_OE, HIGH);
  for (int b = 0; b < BYTES_PER_ROW; b++) shiftByte(~fb[scanStep][b]);
  for (int b = 0; b < BYTES_PER_ROW; b++) shiftByte(~fb[scanStep+8][b]);

  digitalWrite(PIN_STB, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_STB, LOW);

  digitalWrite(PIN_A, (scanStep>>0)&1);
  digitalWrite(PIN_B, (scanStep>>1)&1);
  digitalWrite(PIN_C, (scanStep>>2)&1);
  digitalWrite(PIN_OE, LOW);

  scanStep = (scanStep+1) % SCAN_LINES;
}

inline void shiftByte(byte b) {
  for (int i=7; i>=0; i--) {
    digitalWrite(PIN_DATA, (b>>i)&1);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_CLK, LOW);
  }
}
