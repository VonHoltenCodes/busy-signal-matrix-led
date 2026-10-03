// Software framebuffer + drawing primitives for the scene engine.
//
// Scenes draw into a plain RGB888 buffer here rather than calling
// Adafruit_GFX, for two reasons: alpha blending needs to read pixels back
// (Protomatter cannot), and keeping our own primitives means the browser
// simulator in sim/ and the panel produce identical pixels. gBlit() pushes
// the finished frame to the matrix once per frame.
#pragma once
#include <Arduino.h>
#include <Adafruit_Protomatter.h>

#define FB_W 64
#define FB_H 32

extern uint8_t gFb[FB_W * FB_H * 3];

// Colors are packed 0xRRGGBB. Channels clamp - they must not wrap, or a
// scaled highlight silently turns into a different hue.
// Takes floats: gMix/gScale must round exactly like the simulator, so the
// rounding has to happen here rather than in a caller's int cast.
uint32_t gC(float r, float g, float b);
static inline uint8_t gCr(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t gCg(uint32_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t gCb(uint32_t c) { return c & 0xFF; }

void gSetClip(int x0, int y0, int x1, int y1);
void gResetClip();

void gPx(float x, float y, uint32_t c);
void gPxa(float x, float y, uint32_t c, float a);
void gClear(uint32_t c);
void gRect(float x, float y, float w, float h, uint32_t c);
void gEll(float cx, float cy, float rx, float ry, uint32_t c);
void gCircle(float cx, float cy, float r, uint32_t c);
void gTri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c);
void gQuad(float x0, float y0, float x1, float y1, float x2, float y2,
           float x3, float y3, uint32_t c);
void gLine(float x0, float y0, float x1, float y1, uint32_t c, float t0, float t1);
static inline void gLine(float x0, float y0, float x1, float y1, uint32_t c, float t) {
  gLine(x0, y0, x1, y1, c, t, t);
}

float gHash(int32_t n);
float gHash2(int32_t a, int32_t b);
uint32_t gMix(uint32_t c0, uint32_t c1, float f);
uint32_t gScale(uint32_t c, float f);
float gClamp(float v, float lo, float hi);
float gSmooth(float f);
float gPulse(float t, float a, float b);

int gTextWidth(const char *s, int scale);
void gText(const char *s, float x, float y, int scale, uint32_t c);
void gTextCentered(const char *s, float y, int scale, uint32_t c);

void gBlit(Adafruit_Protomatter &m);
