/*
  =====================================================
  Basketball Scoreboard — NodeMCU (ESP8266) WiFi Client
  =====================================================
  Connects to PC hotspot, fetches live game state from
  nodemcu_server.py and prints to Serial Monitor.

  BOARD    : NodeMCU 1.0 (ESP-12E Module)
  LIBRARY  : ArduinoJson  → Install via Library Manager
             (search "ArduinoJson" by Benoit Blanchon, install v6.x)

  STEPS:
    1. Install "esp8266" board in Arduino IDE:
         File → Preferences → Additional Boards URL:
         https://arduino.esp8266.com/stable/package_esp8266com_index.json
         Then: Tools → Board Manager → search "esp8266" → install

    2. Install ArduinoJson library (v6.x)

    3. Fill in WIFI_SSID, WIFI_PASS, SERVER_IP below

    4. Upload → open Serial Monitor at 115200 baud

  HOW TO FIND YOUR PC HOTSPOT IP:
    - Enable Mobile Hotspot on Windows 11
    - Run nodemcu_server.py on PC
    - It will print the IP on startup
    - Usually 192.168.137.1 for Windows hotspot
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

// ── WiFi & Server settings — CHANGE THESE ─────────────────────
const char* WIFI_SSID   = "YOUR_HOTSPOT_NAME";   // Windows hotspot SSID
const char* WIFI_PASS   = "YOUR_HOTSPOT_PASSWORD";
const char* SERVER_IP   = "192.168.137.1";        // PC hotspot IP (run nodemcu_server.py to confirm)
const int   SERVER_PORT = 8765;

// ── Timing ─────────────────────────────────────────────────────
unsigned long lastFetch = 0;
unsigned long lastPing  = 0;
const int FETCH_INTERVAL = 500;    // ms — fetch game state
const int PING_INTERVAL  = 2000;   // ms — heartbeat to server

// ── WiFi client ────────────────────────────────────────────────
WiFiClient wifiClient;

// ── Setup ──────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("========================================");
  Serial.println("  HIVE Basketball Scoreboard — NodeMCU");
  Serial.println("  Sri Ramakrishna Institute of Technology");
  Serial.println("========================================");
  Serial.println();

  connectWiFi();
}

// ── Main loop ──────────────────────────────────────────────────
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Disconnected — reconnecting...");
    connectWiFi();
    return;
  }

  unsigned long now = millis();

  // ── Fetch game state ──────────────────────────────────────────
  if (now - lastFetch >= FETCH_INTERVAL) {
    lastFetch = now;
    fetchAndDisplay();
  }

  // ── Send heartbeat (ping) ─────────────────────────────────────
  if (now - lastPing >= PING_INTERVAL) {
    lastPing = now;
    sendPing();
  }
}

// ── WiFi connection ───────────────────────────────────────────
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to  %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("[WiFi] Connected! NodeMCU IP: ");
    Serial.println(WiFi.localIP());
    Serial.printf("[WiFi] Server: http://%s:%d\n\n", SERVER_IP, SERVER_PORT);
  } else {
    Serial.println("\n[WiFi] FAILED — check SSID/password and retry");
  }
}

// ── Fetch game state and print to serial ─────────────────────
void fetchAndDisplay() {
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/state";

  HTTPClient http;
  http.begin(wifiClient, url);
  http.setTimeout(1000);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("[HTTP] GET failed, code: %d\n", code);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  // ── Parse JSON ───────────────────────────────────────────────
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[JSON] Parse error: ");
    Serial.println(err.c_str());
    return;
  }

  int  scoreA  = doc["sa"] | 0;
  int  scoreB  = doc["sb"] | 0;
  const char* nameA  = doc["na"] | "A";
  const char* nameB  = doc["nb"] | "B";
  int  clockInt= doc["ci"] | 0;
  int  clockTen= doc["ct"] | 0;
  int  quarter = doc["q"]  | 1;
  bool running = doc["run"] == 1;
  bool gameOver= doc["go"]  == 1;
  bool overtime= doc["ot"]  == 1;
  const char* poss = doc["pos"] | "N";
  int  foulA   = doc["fa"] | 0;
  int  foulB   = doc["fb"] | 0;

  // ── Build clock string ────────────────────────────────────────
  char clockStr[12];
  if (clockInt >= 60) {
    int m = clockInt / 60;
    int s = clockInt % 60;
    sprintf(clockStr, "%02d:%02d.%d", m, s, clockTen);
  } else {
    sprintf(clockStr, "%02d.%d", clockInt, clockTen);
  }

  // ── Period label ──────────────────────────────────────────────
  char periodLabel[8];
  if (overtime) {
    sprintf(periodLabel, "OT%d", quarter > 4 ? quarter - 4 : 1);
  } else {
    sprintf(periodLabel, "Q%d", quarter);
  }

  // ── Print scoreboard ─────────────────────────────────────────
  Serial.println("========================================");
  Serial.printf("  %-10s %3d  |  %-3d %-10s\n", nameA, scoreA, scoreB, nameB);
  Serial.println("----------------------------------------");
  Serial.printf("  Clock : %-10s  Period: %s\n", clockStr, periodLabel);
  Serial.printf("  Fouls : %-2d (A)          %-2d (B)\n", foulA, foulB);
  Serial.printf("  Poss  : %s              Status: %s\n",
                strcmp(poss, "A") == 0 ? nameA :
                strcmp(poss, "B") == 0 ? nameB : "---",
                gameOver ? "GAME OVER" : running ? "RUNNING " : "PAUSED  ");
  Serial.println("========================================");
  Serial.println();
}

// ── Send heartbeat to server ──────────────────────────────────
void sendPing() {
  String url = String("http://") + SERVER_IP + ":" + SERVER_PORT + "/ping";

  HTTPClient http;
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST("{}");
  http.end();

  if (code != 200) {
    Serial.printf("[Ping] Failed (code %d) — server offline?\n", code);
  }
}
