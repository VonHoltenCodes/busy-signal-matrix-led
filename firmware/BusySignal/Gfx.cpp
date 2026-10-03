#include "Gfx.h"
#include "Font5x7.h"
#include <math.h>

uint8_t gFb[FB_W * FB_H * 3];

static int clipX0 = 0, clipY0 = 0, clipX1 = FB_W - 1, clipY1 = FB_H - 1;

// JS Math.round rounds half up, including for negatives; roundf() rounds
// half away from zero. Match the simulator exactly.
static inline int jsRound(float v) { return (int)floorf(v + 0.5f); }

static inline int cl8(float v) {
  int i = jsRound(v);
  return i < 0 ? 0 : (i > 255 ? 255 : i);
}

uint32_t gC(float r, float g, float b) {
  return ((uint32_t)cl8(r) << 16) | ((uint32_t)cl8(g) << 8) | (uint32_t)cl8(b);
}

void gSetClip(int x0, int y0, int x1, int y1) { clipX0 = x0; clipY0 = y0; clipX1 = x1; clipY1 = y1; }
void gResetClip() { gSetClip(0, 0, FB_W - 1, FB_H - 1); }

void gPx(float xf, float yf, uint32_t c) {
  int x = jsRound(xf), y = jsRound(yf);
  if (x < clipX0 || x > clipX1 || y < clipY0 || y > clipY1) return;
  uint8_t *p = &gFb[(y * FB_W + x) * 3];
  p[0] = gCr(c); p[1] = gCg(c); p[2] = gCb(c);
}

void gPxa(float xf, float yf, uint32_t c, float a) {
  int x = jsRound(xf), y = jsRound(yf);
  if (x < clipX0 || x > clipX1 || y < clipY0 || y > clipY1) return;
  if (a <= 0) return;
  if (a > 1) a = 1;
  uint8_t *p = &gFb[(y * FB_W + x) * 3];
  p[0] = (uint8_t)(p[0] + (gCr(c) - (int)p[0]) * a);
  p[1] = (uint8_t)(p[1] + (gCg(c) - (int)p[1]) * a);
  p[2] = (uint8_t)(p[2] + (gCb(c) - (int)p[2]) * a);
}

void gClear(uint32_t c) {
  for (int y = 0; y < FB_H; y++)
    for (int x = 0; x < FB_W; x++) gPx((float)x, (float)y, c);
}

void gRect(float x, float y, float w, float h, uint32_t c) {
  int x0 = jsRound(x), y0 = jsRound(y);
  int x1 = x0 + jsRound(w), y1 = y0 + jsRound(h);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) gPx((float)xx, (float)yy, c);
}

void gEll(float cx, float cy, float rx, float ry, uint32_t c) {
  if (rx < 0.5f) rx = 0.5f;
  if (ry < 0.5f) ry = 0.5f;
  for (int y = (int)ceilf(cy - ry); y <= (int)floorf(cy + ry); y++) {
    float dy = (y - cy) / ry;
    float k = 1.0f - dy * dy;
    if (k < 0) continue;
    float dx = rx * sqrtf(k);
    for (int x = (int)ceilf(cx - dx); x <= (int)floorf(cx + dx); x++) gPx((float)x, (float)y, c);
  }
}

void gCircle(float cx, float cy, float r, uint32_t c) { gEll(cx, cy, r, r, c); }

void gTri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
  int minx = (int)floorf(fminf(x0, fminf(x1, x2)));
  int maxx = (int)ceilf(fmaxf(x0, fmaxf(x1, x2)));
  int miny = (int)floorf(fminf(y0, fminf(y1, y2)));
  int maxy = (int)ceilf(fmaxf(y0, fmaxf(y1, y2)));
  if (minx < clipX0 - 1) minx = clipX0 - 1;
  if (maxx > clipX1 + 1) maxx = clipX1 + 1;
  if (miny < clipY0 - 1) miny = clipY0 - 1;
  if (maxy > clipY1 + 1) maxy = clipY1 + 1;
  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      float sx = x + 0.5f, sy = y + 0.5f;
      float e0 = (sx - x0) * (y1 - y0) - (sy - y0) * (x1 - x0);
      float e1 = (sx - x1) * (y2 - y1) - (sy - y1) * (x2 - x1);
      float e2 = (sx - x2) * (y0 - y2) - (sy - y2) * (x0 - x2);
      if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0))
        gPx((float)x, (float)y, c);
    }
  }
}

void gQuad(float x0, float y0, float x1, float y1, float x2, float y2,
           float x3, float y3, uint32_t c) {
  gTri(x0, y0, x1, y1, x2, y2, c);
  gTri(x0, y0, x2, y2, x3, y3, c);
}

void gLine(float x0, float y0, float x1, float y1, uint32_t c, float t0, float t1) {
  float len = hypotf(x1 - x0, y1 - y0);
  int steps = (int)ceilf(len * 2.0f);
  if (steps < 1) steps = 1;
  for (int i = 0; i <= steps; i++) {
    float f = (float)i / steps;
    float x = x0 + (x1 - x0) * f, y = y0 + (y1 - y0) * f;
    float t = t0 + (t1 - t0) * f;
    if (t <= 1.2f) gPx(x, y, c); else gCircle(x, y, t / 2.0f, c);
  }
}

float gHash(int32_t n0) {
  uint32_t n = ((uint32_t)n0) & 0x7fffffffu;
  n = (n ^ 61u) ^ (n >> 16);
  n = (n + (n << 3)) & 0x7fffffffu;
  n = n ^ (n >> 4);
  n = (n * 0x27d4eb2du) & 0x7fffffffu;
  n = n ^ (n >> 15);
  return (float)(n & 0x7fffffffu) / 2147483647.0f;
}

float gHash2(int32_t a, int32_t b) {
  uint32_t ua = (uint32_t)((int64_t)a * 73856093LL);
  uint32_t ub = (uint32_t)((int64_t)b * 19349663LL);
  return gHash((int32_t)(ua ^ ub));
}

uint32_t gMix(uint32_t c0, uint32_t c1, float f) {
  if (f < 0) f = 0;
  if (f > 1) f = 1;
  return gC(gCr(c0) + ((int)gCr(c1) - (int)gCr(c0)) * f,
            gCg(c0) + ((int)gCg(c1) - (int)gCg(c0)) * f,
            gCb(c0) + ((int)gCb(c1) - (int)gCb(c0)) * f);
}

uint32_t gScale(uint32_t c, float f) {
  return gC(gCr(c) * f, gCg(c) * f, gCb(c) * f);
}

float gClamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float gSmooth(float f) { f = gClamp(f, 0, 1); return f * f * (3 - 2 * f); }

float gPulse(float t, float a, float b) {
  if (t < a || t > b) return 0;
  float f = (t - a) / (b - a);
  return sinf(f * (float)M_PI);
}

int gTextWidth(const char *s, int scale) {
  return (int)strlen(s) * 6 * scale - scale;
}

static void gDrawChar(char ch, float x, float y, int scale, uint32_t c) {
  if (ch >= 'a' && ch <= 'z') ch -= 32;
  if (ch < GFONT_FIRST || ch > GFONT_LAST) ch = '?';
  const uint8_t *cols = GFONT[ch - GFONT_FIRST];
  for (int col = 0; col < 5; col++) {
    uint8_t bits = cols[col];
    for (int row = 0; row < 7; row++) {
      if (!((bits >> row) & 1)) continue;
      if (scale == 1) gPx(x + col, y + row, c);
      else gRect(x + col * scale, y + row * scale, scale, scale, c);
    }
  }
}

void gText(const char *s, float x, float y, int scale, uint32_t c) {
  for (int i = 0; s[i]; i++) gDrawChar(s[i], x + i * 6 * scale, y, scale, c);
}

void gTextCentered(const char *s, float y, int scale, uint32_t c) {
  gText(s, (float)jsRound((FB_W - gTextWidth(s, scale)) / 2.0f), y, scale, c);
}

void gBlit(Adafruit_Protomatter &m) {
  const uint8_t *p = gFb;
  for (int y = 0; y < FB_H; y++) {
    for (int x = 0; x < FB_W; x++, p += 3) {
      m.drawPixel(x, y, m.color565(p[0], p[1], p[2]));
    }
  }
}
