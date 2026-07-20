#include <Adafruit_Protomatter.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "secrets.h"

// ---- Matrix Portal M4 HUB75 pin configuration ----
// Reused as-is from Rocket_launch_Rev3.ino - these are the board's fixed
// HUB75 wiring, not general-purpose GPIO.
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 32
#define MATRIX_BIT_DEPTH 4

uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;

Adafruit_Protomatter matrix(
  MATRIX_WIDTH, MATRIX_BIT_DEPTH, 1, rgbPins, 4, addrPins,
  clockPin, latchPin, oePin, false, 1);

// ---- WiFi / HTTP server ----
WiFiServer server(80);

// ---- Display state ----
enum Mode { MODE_AVAILABLE, MODE_BUSY, MODE_MEETING, MODE_CALL, MODE_MESSAGE, MODE_TIMER };
Mode currentMode = MODE_AVAILABLE;
Mode modeBeforeTimer = MODE_AVAILABLE;

String messageText = "";

unsigned long timerEndMillis = 0;
bool timerActive = false;

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.setPins(SPIWIFI_SS, NINA_ACK, NINA_RESETN, NINA_GPIO0, &SPIWIFI);

  int status = WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (status != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
  }

  Serial.println();
  if (status == WL_CONNECTED) {
    Serial.print("Connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed - check secrets.h and try again.");
  }
}

void setup() {
  Serial.begin(9600);

  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    for (;;); // halt on matrix init failure
  }

  connectWiFi();
  server.begin();
}

// ---- HTTP request handling ----
// Minimal GET-only parser: reads the request line, extracts the path and
// query string, dispatches to a handler, replies 200 OK, closes. No need
// for a full HTTP server library since every request is a bare GET with
// no body.
void handleClient(WiFiClient &client);
void applyRoute(const String &path, const String &query);
String urlDecode(const String &input);
String queryParam(const String &query, const String &key);
void renderCurrentMode();

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }

  if (timerActive && millis() >= timerEndMillis) {
    timerActive = false;
    currentMode = modeBeforeTimer;
  }

  renderCurrentMode();
  matrix.show();
}

void handleClient(WiFiClient &client) {
  String requestLine = "";
  unsigned long start = millis();
  while (client.connected() && millis() - start < 1000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n') break;
      if (c != '\r') requestLine += c;
    }
  }

  // Expect: "GET /path?query HTTP/1.1"
  int firstSpace = requestLine.indexOf(' ');
  int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
  if (firstSpace >= 0 && secondSpace > firstSpace) {
    String target = requestLine.substring(firstSpace + 1, secondSpace);
    int qIndex = target.indexOf('?');
    String path = qIndex >= 0 ? target.substring(0, qIndex) : target;
    String query = qIndex >= 0 ? target.substring(qIndex + 1) : "";
    applyRoute(path, query);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();
}

void applyRoute(const String &path, const String &query) {
  if (path == "/available") {
    currentMode = MODE_AVAILABLE;
    timerActive = false;
  } else if (path == "/busy") {
    currentMode = MODE_BUSY;
    timerActive = false;
  } else if (path == "/meeting") {
    currentMode = MODE_MEETING;
    timerActive = false;
  } else if (path == "/call") {
    currentMode = MODE_CALL;
    timerActive = false;
  } else if (path == "/message") {
    messageText = urlDecode(queryParam(query, "text"));
    currentMode = MODE_MESSAGE;
    timerActive = false;
  } else if (path == "/timer") {
    long minutes = queryParam(query, "minutes").toInt();
    if (minutes > 0) {
      modeBeforeTimer = currentMode;
      timerEndMillis = millis() + (minutes * 60000UL);
      timerActive = true;
      currentMode = MODE_TIMER;
    }
  }
}

String queryParam(const String &query, const String &key) {
  int start = query.indexOf(key + "=");
  if (start < 0) return "";
  start += key.length() + 1;
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();
  return query.substring(start, end);
}

String urlDecode(const String &input) {
  String out = "";
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < input.length()) {
      String hex = input.substring(i + 1, i + 3);
      out += (char) strtol(hex.c_str(), nullptr, 16);
      i += 2;
    } else {
      out += c;
    }
  }
  return out;
}

// ---- Rendering ----
// Placeholder fills only. Actual visual design (layouts, fonts, colors,
// animations for meeting/call/message/timer) is pending a design
// discussion - not implemented yet on purpose.
void renderCurrentMode() {
  matrix.fillScreen(0);
  switch (currentMode) {
    case MODE_AVAILABLE:
      matrix.fillScreen(matrix.color565(0, 255, 0));
      break;
    case MODE_BUSY:
    case MODE_MEETING:
    case MODE_CALL:
      matrix.fillScreen(matrix.color565(255, 0, 0));
      break;
    case MODE_MESSAGE:
    case MODE_TIMER:
      // TODO: pending design discussion
      break;
  }
}
