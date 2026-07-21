#include <Adafruit_Protomatter.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <math.h>
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

// ---- Status definitions ----
// Mirrors the STATUSES table validated in the browser simulator: group
// drives the color/status-slide background, label is what's drawn (short
// enough to always fit at scale 1), path is the HTTP route that selects it.
enum StatusKey { ST_MEETING, ST_CALL, ST_RACING, ST_RECORDING, ST_WORKING, ST_COMEIN, ST_COUNT };
enum Group { GROUP_BUSY, GROUP_AVAILABLE };

struct StatusDef {
  Group group;
  const char *label;
  const char *path;
};

const StatusDef STATUS_DEFS[ST_COUNT] = {
  { GROUP_BUSY,      "MEETING", "/meeting"   },
  { GROUP_BUSY,      "ON CALL", "/call"      },
  { GROUP_BUSY,      "RACING",  "/racing"    },
  { GROUP_BUSY,      "REC",     "/recording" },
  { GROUP_AVAILABLE, "WORKING", "/working"   },
  { GROUP_AVAILABLE, "COME IN", "/comein"    },
};

// ---- Display state ----
enum DisplayMode { DISP_NORMAL, DISP_MESSAGE, DISP_TIMER_STANDALONE };
// NORMAL alternates the color slide and status slide for currentStatus every
// 5s. If a timer is active and attached, the pizza+countdown takes the
// status-slide beat instead of the plain icon+label. TIMER_STANDALONE is a
// full-screen countdown that doesn't participate in that cycle at all.

DisplayMode displayMode = DISP_NORMAL;
StatusKey currentStatus = ST_WORKING;
String messageText = "";

bool timerActive = false;
bool timerAttached = false;
unsigned long timerTotalSeconds = 0;
unsigned long timerEndMillis = 0;

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
void render();
float timerRemainingSeconds();

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }

  if (timerActive && millis() >= timerEndMillis) {
    timerActive = false;
    if (displayMode == DISP_TIMER_STANDALONE) displayMode = DISP_NORMAL;
  }

  render();
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
  for (int i = 0; i < ST_COUNT; i++) {
    if (path == STATUS_DEFS[i].path) {
      currentStatus = (StatusKey)i;
      displayMode = DISP_NORMAL;
      timerActive = false;
      return;
    }
  }

  if (path == "/message") {
    messageText = urlDecode(queryParam(query, "text"));
    displayMode = DISP_MESSAGE;
  } else if (path == "/message/clear") {
    if (displayMode == DISP_MESSAGE) displayMode = DISP_NORMAL;
  } else if (path == "/timer") {
    long minutes = queryParam(query, "minutes").toInt();
    if (minutes > 0) {
      bool attached = queryParam(query, "attached") != "0";
      timerTotalSeconds = (unsigned long)minutes * 60UL;
      timerEndMillis = millis() + timerTotalSeconds * 1000UL;
      timerActive = true;
      timerAttached = attached;
      displayMode = attached ? DISP_NORMAL : DISP_TIMER_STANDALONE;
    }
  } else if (path == "/timer/cancel") {
    timerActive = false;
    if (displayMode == DISP_TIMER_STANDALONE) displayMode = DISP_NORMAL;
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

// ---------------------------------------------------------------------
// Draw primitives - thin float-friendly wrappers around Adafruit_GFX so
// icon code (ported straight from the browser simulator) can keep using
// the same fractional coordinates it was designed with.
// ---------------------------------------------------------------------
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return matrix.color565(r, g, b);
}

void fillCircleF(float cx, float cy, float r, uint16_t color) {
  int16_t ir = (int16_t)roundf(r);
  if (ir < 1) ir = 1;
  matrix.fillCircle((int16_t)roundf(cx), (int16_t)roundf(cy), ir, color);
}

void fillRectF(float x, float y, float w, float h, uint16_t color) {
  matrix.fillRect((int16_t)roundf(x), (int16_t)roundf(y), (int16_t)roundf(w), (int16_t)roundf(h), color);
}

void fillTriangleF(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color) {
  matrix.fillTriangle((int16_t)roundf(x0), (int16_t)roundf(y0), (int16_t)roundf(x1), (int16_t)roundf(y1),
                       (int16_t)roundf(x2), (int16_t)roundf(y2), color);
}

// Adafruit_GFX has no thick-line primitive, so this stamps a small filled
// circle at each step of a Bresenham line - same technique the simulator
// used, just in integer/float C++ instead of JS.
void thickLineF(float x0, float y0, float x1, float y1, uint16_t color, float thick) {
  int ix0 = (int)roundf(x0), iy0 = (int)roundf(y0);
  int ix1 = (int)roundf(x1), iy1 = (int)roundf(y1);
  int dx = abs(ix1 - ix0), sx = ix0 < ix1 ? 1 : -1;
  int dy = -abs(iy1 - iy0), sy = iy0 < iy1 ? 1 : -1;
  int err = dx + dy;
  int guard = 0;
  while (guard++ < 200) {
    if (thick <= 1) matrix.drawPixel(ix0, iy0, color);
    else fillCircleF(ix0, iy0, thick / 2.0f, color);
    if (ix0 == ix1 && iy0 == iy1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; ix0 += sx; }
    if (e2 <= dx) { err += dx; iy0 += sy; }
  }
}

// ---------------------------------------------------------------------
// Text - Adafruit_GFX's built-in font is the same 5x7-glyph/6px-advance
// shape the simulator's hand-rolled font was designed to match, so labels
// port over with just a width formula, no font table needed.
// ---------------------------------------------------------------------
int16_t textWidth(const String &s, uint8_t scale) {
  return (int16_t)s.length() * 6 * scale - scale;
}

void drawCenteredText(const String &text, uint16_t color, int16_t maxWidth, int16_t bandTop, int16_t bandHeight, bool preferScale2) {
  uint8_t scale = preferScale2 ? 2 : 1;
  int16_t w = textWidth(text, scale);
  if (w > maxWidth) { scale = 1; w = textWidth(text, scale); }
  int16_t h = 7 * scale;
  int16_t x = (MATRIX_WIDTH - w) / 2;
  int16_t y = bandTop + (bandHeight - h) / 2;
  matrix.setTextSize(scale);
  matrix.setTextColor(color);
  matrix.setCursor(x, y);
  matrix.print(text);
}

// ---------------------------------------------------------------------
// Bulb border (retro marquee sign frame)
// ---------------------------------------------------------------------
void drawBulbBorder(uint16_t color) {
  for (int x = 1; x < MATRIX_WIDTH - 1; x += 5) {
    matrix.drawPixel(x, 0, color);
    matrix.drawPixel(x, MATRIX_HEIGHT - 1, color);
  }
  for (int y = 1; y < MATRIX_HEIGHT - 2; y += 5) {
    matrix.drawPixel(0, y, color);
    matrix.drawPixel(MATRIX_WIDTH - 1, y, color);
  }
}

// ---------------------------------------------------------------------
// Status icons - ported 1:1 from the simulator's ICONS table.
// ---------------------------------------------------------------------
void iconMeeting(int16_t cx, int16_t cy) {
  int16_t deskY = cy + 6;
  uint16_t white = rgb(244, 244, 240);
  fillRectF(cx - 10, deskY, 21, 1, white);
  fillRectF(cx - 9, deskY + 1, 1, 3, white);
  fillRectF(cx + 8, deskY + 1, 1, 3, white);
  fillRectF(cx - 2, deskY - 2, 8, 2, white);
  fillRectF(cx + 4, deskY - 8, 2, 8, white);
  fillCircleF(cx - 7, cy - 6, 2.2, rgb(224, 178, 138));
  fillRectF(cx - 9, cy - 3, 4, 9, rgb(90, 160, 230));
  thickLineF(cx - 6, cy, cx - 1, deskY - 3, rgb(90, 160, 230), 1.3);
}

void iconCall(int16_t cx, int16_t cy, uint16_t bg) {
  cy += 2;
  int16_t dialCy = cy - 2;
  uint16_t body = rgb(225, 205, 165);
  uint16_t dial = rgb(40, 40, 44);
  fillRectF(cx - 8, cy - 1, 16, 7, body);
  fillCircleF(cx, dialCy, 5.5, dial);
  for (int i = 0; i < 8; i++) {
    float a = i * (PI / 4);
    fillCircleF(cx + cosf(a) * 3.6, dialCy + sinf(a) * 3.6, 0.9, bg);
  }
  fillCircleF(cx - 7, cy - 8, 2, body);
  fillCircleF(cx + 7, cy - 8, 2, body);
  fillRectF(cx - 7, cy - 9, 15, 2, body);
}

void iconRacing(int16_t cx, int16_t cy) {
  uint16_t blue = rgb(30, 70, 190);
  uint16_t gold = rgb(212, 175, 55);
  uint16_t glass = rgb(196, 206, 214);
  uint16_t tire = rgb(15, 15, 18);
  uint16_t dark = rgb(20, 20, 24);
  uint16_t white = rgb(244, 244, 240);

  fillRectF(cx - 11, cy, 22, 5, blue);
  fillRectF(cx - 4, cy - 6, 13, 6, blue);
  fillRectF(cx - 2, cy - 5, 9, 3, glass);
  fillRectF(cx + 9, cy - 6, 2, 6, gold);
  fillRectF(cx + 12, cy - 6, 2, 6, gold);
  fillRectF(cx + 7, cy - 8, 9, 2, gold);

  int16_t wheelX[2] = { (int16_t)(cx - 9), (int16_t)(cx + 9) };
  for (int i = 0; i < 2; i++) {
    fillCircleF(wheelX[i], cy + 5, 4, tire);
    fillCircleF(wheelX[i], cy + 5, 2.4, gold);
    fillCircleF(wheelX[i], cy + 5, 0.8, dark);
  }

  fillCircleF(cx - 1, cy + 2, 2.2, white);
  fillCircleF(cx - 11, cy + 1, 1.1, rgb(255, 240, 180));

  int16_t starX[3] = { (int16_t)(cx - 5), (int16_t)(cx + 3), (int16_t)(cx - 1) };
  int16_t starY[3] = { (int16_t)(cy + 1), (int16_t)(cy + 2), (int16_t)(cy - 2) };
  for (int i = 0; i < 3; i++) {
    int16_t x = starX[i], y = starY[i];
    matrix.drawPixel(x, y, gold);
    matrix.drawPixel(x - 1, y, gold);
    matrix.drawPixel(x + 1, y, gold);
    matrix.drawPixel(x, y - 1, gold);
    matrix.drawPixel(x, y + 1, gold);
  }
}

void iconRecording(int16_t cx, int16_t cy) {
  uint16_t camBody = rgb(64, 66, 72);
  uint16_t dark = rgb(20, 20, 24);
  uint16_t recRed = rgb(255, 60, 55);
  fillRectF(cx - 7, cy - 4, 12, 8, camBody);
  fillRectF(cx - 4, cy - 7, 4, 3, camBody);
  fillCircleF(cx + 6, cy, 3.4, dark);
  fillCircleF(cx + 6, cy, 2.2, recRed);
  fillCircleF(cx - 5, cy - 2, 1, recRed);
}

void iconWorking(int16_t cx, int16_t cy) {
  // Badge is STATUS_BG_AVAIL blended 18% toward CHECK_GREEN, pre-computed
  // since working only ever renders on that fixed background.
  uint16_t badge = rgb(22, 71, 41);
  uint16_t green = rgb(70, 210, 130);
  fillCircleF(cx, cy, 9, badge);
  thickLineF(cx - 5, cy + 1, cx - 1, cy + 6, green, 2.2);
  thickLineF(cx - 1, cy + 6, cx + 7, cy - 6, green, 2.2);
}

void iconComeIn(int16_t cx, int16_t cy) {
  uint16_t brown = rgb(140, 95, 55);
  uint16_t green = rgb(70, 210, 130);
  fillRectF(cx - 9, cy - 9, 2, 18, brown);
  fillRectF(cx - 9, cy - 9, 10, 2, brown);
  fillRectF(cx - 6, cy - 1, 8, 2, green);
  fillTriangleF(cx + 2, cy - 5, cx + 2, cy + 5, cx + 9, cy, green);
}

void drawStatusIcon(StatusKey key, int16_t cx, int16_t cy, uint16_t bg) {
  switch (key) {
    case ST_MEETING:   iconMeeting(cx, cy); break;
    case ST_CALL:      iconCall(cx, cy, bg); break;
    case ST_RACING:    iconRacing(cx, cy); break;
    case ST_RECORDING: iconRecording(cx, cy); break;
    case ST_WORKING:   iconWorking(cx, cy); break;
    case ST_COMEIN:    iconComeIn(cx, cy); break;
    default: break;
  }
}

// ---------------------------------------------------------------------
// Scene renderers
// ---------------------------------------------------------------------
uint16_t statusBg(Group group) {
  return group == GROUP_BUSY ? rgb(46, 14, 14) : rgb(12, 40, 22);
}

void renderColorSlide() {
  const StatusDef &def = STATUS_DEFS[currentStatus];
  uint16_t bg = def.group == GROUP_BUSY ? rgb(222, 30, 30) : rgb(32, 190, 96);
  matrix.fillScreen(bg);
  drawBulbBorder(rgb(255, 176, 70));
  drawCenteredText(def.label, rgb(244, 244, 240), 58, 2, 28, true);
}

void renderStatusSlide() {
  const StatusDef &def = STATUS_DEFS[currentStatus];
  uint16_t bg = statusBg(def.group);
  matrix.fillScreen(bg);
  drawBulbBorder(rgb(255, 176, 70));
  drawStatusIcon(currentStatus, 32, 11, bg);
  drawCenteredText(def.label, rgb(244, 244, 240), 58, 22, 9, false);
}

void renderMessageSlide() {
  const StatusDef &def = STATUS_DEFS[currentStatus];
  matrix.fillScreen(statusBg(def.group));
  drawBulbBorder(rgb(255, 176, 70));

  String text = messageText.length() ? messageText : " ";
  int16_t w = textWidth(text, 2);
  float cyclePos = fmodf((millis() / 1000.0f) * 22.0f, (float)(w + MATRIX_WIDTH));
  int16_t x = (int16_t)roundf(MATRIX_WIDTH - cyclePos);

  matrix.setTextSize(2);
  matrix.setTextColor(rgb(255, 176, 70));
  matrix.setCursor(x, 9);
  matrix.print(text);
}

int wedgeIndexOf(float dx, float dy) {
  float ang = atan2f(dx, -dy);
  if (ang < 0) ang += 2 * PI;
  return ((int)(ang / (PI / 4))) % 8;
}

String fmtClock(float seconds) {
  long s = (long)roundf(seconds);
  if (s < 0) s = 0;
  int mm = s / 60, ss = s % 60;
  char buf[8]; // up to "999:59\0" - the 5-min-increment UI tops out at 120 min, "120:00" needs 7
  snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
  return String(buf);
}

float timerRemainingSeconds() {
  if (!timerActive) return 0;
  long remainMs = (long)(timerEndMillis - millis());
  if (remainMs < 0) remainMs = 0;
  return remainMs / 1000.0f;
}

void renderTimerPizza() {
  uint16_t neutralBg = rgb(5, 5, 8);
  matrix.fillScreen(neutralBg);
  drawBulbBorder(rgb(255, 176, 70));

  // Countdown text occupies rows 1-7, so the pizza starts below that.
  const int16_t cx = 32, cy = 19, R = 10;
  matrix.fillCircle(cx, cy, R, rgb(196, 142, 72));      // crust
  matrix.fillCircle(cx, cy, R - 2, rgb(176, 46, 26));   // sauce
  matrix.fillCircle(cx, cy, R - 3, rgb(242, 190, 72));  // cheese

  float remaining = timerRemainingSeconds();
  float total = (float)timerTotalSeconds;
  float fraction = total > 0 ? remaining / total : 0;
  int slicesEaten = (int)floorf((1 - fraction) * 8 + 1e-6f);
  if (slicesEaten < 0) slicesEaten = 0;
  if (slicesEaten > 8) slicesEaten = 8;

  uint16_t pepperoni = rgb(150, 36, 32);
  float pepDist = R * 0.55f;
  for (int i = 0; i < 8; i++) {
    if (i < slicesEaten) continue;
    float ang = (i + 0.5f) * (PI / 4);
    fillCircleF(cx + sinf(ang) * pepDist, cy - cosf(ang) * pepDist, 1.0f, pepperoni);
  }

  if (slicesEaten > 0) {
    int bound = R + 1;
    for (int y = cy - bound; y <= cy + bound; y++) {
      for (int x = cx - bound; x <= cx + bound; x++) {
        float dx = x - cx + 0.5f, dy = y - cy + 0.5f;
        if (dx * dx + dy * dy > (R + 0.6f) * (R + 0.6f)) continue;
        if (wedgeIndexOf(dx, dy) < slicesEaten) matrix.drawPixel(x, y, neutralBg);
      }
    }
  }

  drawCenteredText(fmtClock(remaining), rgb(255, 176, 70), 60, 1, 7, false);
}

void render() {
  if (displayMode == DISP_MESSAGE) {
    renderMessageSlide();
  } else if (displayMode == DISP_TIMER_STANDALONE) {
    renderTimerPizza();
  } else {
    bool colorPhase = ((millis() / 5000) % 2) == 0;
    if (colorPhase) {
      renderColorSlide();
    } else if (timerActive && timerAttached) {
      renderTimerPizza();
    } else {
      renderStatusSlide();
    }
  }
}
