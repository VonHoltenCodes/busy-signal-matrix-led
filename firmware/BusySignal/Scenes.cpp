#include "Scenes.h"
#include "Gfx.h"
#include <math.h>

// ---- Palette ----
static const uint32_t AMBER     = 0xFFA828;
static const uint32_t AMBER_HOT = 0xFFE28C;
static const uint32_t AMBER_DIM = 0x965C12;
static const uint32_t JP_RED    = 0xC6261E;
static const uint32_t JP_ORANGE = 0xE9701A;
static const uint32_t NIGHT     = 0x04060C;
static const uint32_t JUNGLE    = 0x07160D;
static const uint32_t WOOD      = 0x683E1C;
static const uint32_t WOOD_DK   = 0x3A210E;
static const uint32_t WOOD_LT   = 0x8C582A;
static const uint32_t STONE     = 0x4E443A;
static const uint32_t STONE_DK  = 0x2A241E;
static const uint32_t STONE_LT  = 0x706458;
static const uint32_t BONE      = 0xF0E6CE;
static const uint32_t BLACK     = 0x000000;
static const uint32_t SIL       = 0x060508;
static const uint32_t SIL_FAR   = 0x1A1210;
static const uint32_t WHITE     = 0xF4F4F0;

struct Pt { float x, y; };

static Pt rotp(float x, float y, float cx, float cy, float a) {
  float s = sinf(a), c = cosf(a);
  float dx = x - cx, dy = y - cy;
  return { cx + dx * c - dy * s, cy + dx * s + dy * c };
}

static inline float jsMod(float a, float b) { return fmodf(a, b); }

// =====================================================================
// Shared set pieces
// =====================================================================

static void sunsetSky(int horizon) {
  for (int y = 0; y < horizon; y++) {
    float f = (float)y / (float)max(1, horizon - 1);
    uint32_t c = gMix(gC(52, 8, 26), gC(246, 138, 28), f * f);
    for (int x = 0; x < FB_W; x++) gPx(x, y, c);
  }
}

static void nightSky(float t) {
  gClear(NIGHT);
  for (int i = 0; i < 26; i++) {
    int x = (int)floorf(gHash(i * 7 + 1) * FB_W);
    int y = (int)floorf(gHash(i * 13 + 5) * 18);
    float tw = 0.35f + 0.65f * fabsf(sinf(t * 1.4f + i));
    gPxa(x, y, gC(200, 220, 255), 0.25f + tw * 0.5f);
  }
}

static void treeline(int topY, uint32_t col, int seed) {
  for (int x = 0; x < FB_W; x++) {
    float n = gHash2(x + seed, seed) * 3 + gHash2((int)floorf(x / 5.0f) + seed, seed * 3) * 3;
    int top = topY + (int)floorf(n) - 2;
    for (int y = top; y < FB_H; y++) gPx(x, y, col);
  }
  for (int i = 0; i < 5; i++) {
    float fx = 4 + floorf(gHash(i * 31 + seed) * (FB_W - 8));
    float fh = 5 + floorf(gHash(i * 17 + seed) * 5);
    gLine(fx, topY + 2, fx - 2, topY + 2 - fh, col, 1.6f, 1);
    for (int k = 1; k <= 3; k++) {
      float fy = topY + 2 - (fh * k) / 4;
      gLine(fx - k * 0.5f, fy, fx - k * 0.5f - 3, fy - 1.5f, col, 1.2f, 1);
      gLine(fx - k * 0.5f, fy, fx - k * 0.5f + 3, fy - 1.5f, col, 1.2f, 1);
    }
  }
}

static void torch(float cx, float baseY, float t, int seed) {
  float f = gHash2((int)floorf(t * 14) + seed, seed);
  float f2 = gHash2((int)floorf(t * 9) + seed * 3, seed);
  float hgt = 5.0f + f * 2.2f;
  for (int i = 0; i <= 10; i++) {
    float k = i / 10.0f;
    float y = baseY - k * hgt;
    float sway = sinf(t * 6 + seed + k * 3) * (0.8f + k * 1.4f) * (0.6f + f2 * 0.8f);
    float wdt = (1 - k) * 2.3f + 0.5f;
    uint32_t col = gMix(gMix(gC(255, 60, 0), AMBER, k * 0.8f), AMBER_HOT, k * k);
    gCircle(cx + sway * 0.5f, y, wdt, col);
  }
  // cx is fractional, so these must step in floats: an int loop lands on
  // different pixel centres than the simulator and shifts the whole glow.
  for (float y = baseY - 4; y < baseY + 9; y++)
    for (float x = cx - 6; x <= cx + 6; x++) {
      float d = hypotf(x - cx, (y - baseY) * 0.8f);
      if (d < 6.5f) gPxa(x, y, AMBER, (1 - d / 6.5f) * 0.22f * (0.75f + f * 0.5f));
    }
}

// ---- Tyrannosaurus, profile facing left, feet on y=27 ----
static Pt trexBody(float ox, float oy, uint32_t col, uint32_t farCol,
                   float jaw, float lift, float stride) {
  gLine(ox + 40, oy + 13, ox + 57, oy + 7.5f, col, 6, 1.2f);

  float fs = sinf(stride) * 1.6f;
  gEll(ox + 39.5f, oy + 17, 3.2f, 4.0f, farCol);
  gLine(ox + 39.5f, oy + 20, ox + 41.5f + fs, oy + 25.6f, farCol, 2.2f, 1.8f);
  gRect(ox + 40 + fs, oy + 25.6f, 5.5f, 1.5f, farCol);

  gEll(ox + 34, oy + 14, 8, 4.6f, col);

  float ns = sinf(stride + (float)M_PI) * 1.6f;
  gEll(ox + 35.5f, oy + 17.5f, 3.6f, 4.4f, col);
  gLine(ox + 35.5f, oy + 21, ox + 33 + ns, oy + 25.8f, col, 2.6f, 2);
  gRect(ox + 30.5f + ns, oy + 25.6f, 6, 1.7f, col);

  gLine(ox + 29.5f, oy + 14, ox + 27.4f, oy + 16.6f, col, 2.2f, 1.6f);
  gPx(ox + 26.6f, oy + 17.2f, col);

  gLine(ox + 27, oy + 12, ox + 21.5f, oy + 9.4f, col, 5.2f, 4.4f);

  float hx = ox + 20.5f, hy = oy + 10;
  #define R(X, Y)  rotp((X) + ox, (Y) + oy, hx, hy, -lift)
  #define RJ(X, Y) ({ Pt _j = rotp((X) + ox, (Y) + oy, ox + 20, oy + 10.6f, jaw); \
                      rotp(_j.x, _j.y, hx, hy, -lift); })

  Pt a1 = R(9.5f, 8.8f), a2 = R(11.5f, 6.2f), a3 = R(17.5f, 5.4f), a4 = R(21, 8.8f);
  Pt a5 = R(21, 10.1f), a6 = R(9.8f, 10.1f);
  gQuad(a1.x, a1.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y, col);
  gQuad(a1.x, a1.y, a4.x, a4.y, a5.x, a5.y, a6.x, a6.y, col);

  Pt b1 = RJ(10.4f, 10.6f), b2 = RJ(19.4f, 10.3f), b3 = RJ(20, 12.2f), b4 = RJ(11.2f, 12.5f);
  gQuad(b1.x, b1.y, b2.x, b2.y, b3.x, b3.y, b4.x, b4.y, col);

  if (jaw > 0.10f) {
    for (int i = 0; i < 4; i++) {
      float fx = 11.4f + i * 2.2f;
      Pt u = R(fx, 10.1f);
      gTri(u.x, u.y, u.x + 1.1f, u.y, u.x + 0.5f, u.y + 1.4f, BONE);
      Pt l = RJ(fx + 0.7f, 10.7f);
      gTri(l.x, l.y, l.x + 1.1f, l.y, l.x + 0.5f, l.y - 1.4f, BONE);
    }
  }

  Pt e = R(16.6f, 7.4f);
  gPx(e.x, e.y, AMBER_HOT);
  Pt mouth = R(10, 10.6f);
  #undef R
  #undef RJ
  return mouth;
}

// One-pixel amber rim: the same silhouette stamped a pixel higher in dim
// amber, then the black body over it.
static Pt trex(float ox, float oy, float jaw, float lift, float stride) {
  trexBody(ox, oy - 1, AMBER_DIM, gScale(AMBER_DIM, 0.55f), jaw, lift, stride);
  return trexBody(ox, oy, SIL, SIL_FAR, jaw, lift, stride);
}

// Compact rex silhouette, 27x18, feet on oy+17.
static void miniRex(float ox, float oy, uint32_t col, uint32_t far) {
  gLine(ox + 19, oy + 7.5f, ox + 26, oy + 4, col, 4, 1);
  gEll(ox + 18.5f, oy + 11, 2, 2.6f, far);
  gLine(ox + 18.5f, oy + 12.5f, ox + 20, oy + 15.5f, far, 1.6f);
  gRect(ox + 19.5f, oy + 15.6f, 3.5f, 1.2f, far);
  gEll(ox + 15, oy + 8.5f, 5.5f, 3.2f, col);
  gEll(ox + 16, oy + 11, 2.4f, 3, col);
  gLine(ox + 16, oy + 13, ox + 14.5f, oy + 16, col, 1.8f);
  gRect(ox + 12.5f, oy + 15.8f, 4.5f, 1.3f, col);
  gLine(ox + 12, oy + 7, ox + 9.5f, oy + 5.5f, col, 3, 2.6f);
  gQuad(ox + 1, oy + 4.4f, ox + 2.5f, oy + 2.8f, ox + 7, oy + 2.4f, ox + 9.5f, oy + 4.4f, col);
  gQuad(ox + 1, oy + 4.4f, ox + 9.5f, oy + 4.4f, ox + 9.5f, oy + 5.4f, ox + 1.2f, oy + 5.4f, col);
  gQuad(ox + 1.6f, oy + 5.9f, ox + 8.6f, oy + 5.7f, ox + 9, oy + 7, ox + 2.2f, oy + 7.2f, col);
  gLine(ox + 12.5f, oy + 8.5f, ox + 10.8f, oy + 10.3f, col, 1.6f);
}

static void footprint(float cx, float cy, float f) {
  if (f <= 0) return;
  uint32_t dirt = gC(44, 26, 12);
  gEll(cx, cy, 3.2f * f, 2.0f * f, dirt);
  for (int i = -1; i <= 1; i++)
    gTri(cx + i * 2.6f, cy - 1.5f, cx + i * 2.6f - 1.1f, cy - 1.5f,
         cx + i * 3.1f, cy - 2.4f - 2.2f * f, dirt);
  if (f < 1)
    for (float a = 0; a < 6.2f; a += 0.5f)
      gPxa(cx + cosf(a) * (4 + f * 5), cy + sinf(a) * (1.6f + f * 2), gC(150, 120, 84), (1 - f) * 0.6f);
}

// ---- Velociraptor head, profile facing right ----
static void raptorHead(float ox, float oy, float blink, float jaw,
                       uint32_t bodyCol, uint32_t stripeCol) {
  uint32_t dark = gScale(bodyCol, 0.55f);
  gQuad(ox + 0, oy + 16, ox + 18, oy + 1, ox + 24, oy + 9, ox + 6, oy + 20, bodyCol);
  gRect(ox + 0, oy + 12, 8, 10, bodyCol);
  gEll(ox + 26, oy + 6, 10, 7.5f, bodyCol);
  gQuad(ox + 32, oy + 1.8f, ox + 46, oy + 5.6f, ox + 46, oy + 8.6f, ox + 32, oy + 11.5f, bodyCol);

  float jhx = ox + 32, jhy = oy + 11;
  #define J(X, Y) rotp((X), (Y), jhx, jhy, jaw)
  Pt j1 = J(ox + 32, oy + 10), j2 = J(ox + 45, oy + 9.3f),
     j3 = J(ox + 45.5f, oy + 11.6f), j4 = J(ox + 32, oy + 13.5f);
  gQuad(j1.x, j1.y, j2.x, j2.y, j3.x, j3.y, j4.x, j4.y, bodyCol);

  for (int i = 0; i < 6; i++) {
    float fx = ox + 34 + i * 1.9f;
    gPx(fx, oy + 9.4f + i * 0.16f, BONE);
    Pt lt = J(fx + 0.6f, oy + 9.6f);
    gPx(lt.x, lt.y, BONE);
  }
  #undef J

  gLine(ox + 32, oy + 10.6f, ox + 45, oy + 8.8f, dark, 1);
  gQuad(ox + 20, oy + 1.5f, ox + 33, oy + 1.2f, ox + 33, oy + 3.4f, ox + 20, oy + 4.2f, dark);
  if (blink < 0.5f) {
    gCircle(ox + 27, oy + 5.4f, 2.2f, gC(255, 205, 45));
    gRect(ox + 26.6f, oy + 3.8f, 1, 3.4f, gC(20, 12, 6));
    gPx(ox + 28, oy + 4.4f, gC(255, 250, 220));
  } else {
    gCircle(ox + 27, oy + 5.4f, 2.2f, gScale(bodyCol, 0.8f));
    gLine(ox + 25, oy + 5.6f, ox + 29, oy + 5.4f, dark, 1);
  }
  gPx(ox + 43.5f, oy + 6.4f, dark);

  static const float PTS[7][2] = {
    {45, 4.8f}, {38, 2.8f}, {30, 0.9f}, {22, 0.5f}, {13, 5.2f}, {5, 12}, {1, 19}
  };
  uint32_t core = gMix(stripeCol, WHITE, 0.42f);
  for (int i = 0; i < 6; i++)
    gLine(ox + PTS[i][0], oy + PTS[i][1], ox + PTS[i + 1][0], oy + PTS[i + 1][1], stripeCol, 2.6f, 2.6f);
  for (int i = 0; i < 6; i++)
    gLine(ox + PTS[i][0], oy + PTS[i][1] + 0.3f, ox + PTS[i + 1][0], oy + PTS[i + 1][1] + 0.3f, core, 1, 1);

  for (int i = 0; i < 30; i++) {
    float sx = ox + 4 + gHash(i * 11 + 3) * 44;
    float sy = oy + 2 + gHash(i * 19 + 7) * 18;
    gPxa(sx, sy, dark, 0.5f);
  }
}

// ---- JP tour Jeep, driving right ----
static void jeep(float x, float y, float t) {
  const uint32_t CREAM = gC(228, 212, 170);
  const uint32_t CREAM_DK = gC(160, 144, 110);
  const uint32_t STRIPE = gC(208, 60, 32);
  const uint32_t CAGE = gC(48, 50, 54);
  const uint32_t TIRE = gC(16, 16, 20);

  for (int i = 0; i < 6; i++) {
    float age = jsMod(t * 2.2f + i * 0.31f, 1.0f);
    float dx = x + 3 - age * 20, dy = y + 12 - age * 4 - gHash(i * 23) * 2;
    float r = 1.4f + age * 4;
    for (int yy = (int)floorf(dy - r); yy <= dy + r; yy++)
      for (int xx = (int)floorf(dx - r); xx <= dx + r; xx++)
        if (hypotf(xx - dx, yy - dy) <= r) gPxa(xx, yy, gC(152, 124, 90), (1 - age) * 0.40f);
  }

  gRect(x + 7, y - 5, 13, 1, CAGE);
  gRect(x + 7, y - 5, 1, 5, CAGE);
  gLine(x + 19.5f, y, x + 20.5f, y - 5, CAGE, 1.4f);

  gQuad(x + 16, y - 4, x + 19.6f, y - 4, x + 20, y - 0.5f, x + 16, y - 0.5f, gC(118, 142, 150));
  gRect(x + 1, y, 20, 9, CREAM);
  gRect(x + 20, y + 4, 10, 5, CREAM);
  gRect(x + 1, y + 8, 29, 1, CREAM_DK);
  gRect(x + 20, y + 3, 10, 1, CREAM_DK);
  gCircle(x + 0.5f, y + 4, 2.6f, gC(28, 28, 32));
  gCircle(x + 0.5f, y + 4, 1.1f, CREAM_DK);

  gRect(x + 2, y + 7, 28, 1, STRIPE);
  gRect(x + 21, y + 5, 8, 1, STRIPE);
  gRect(x + 19, y, 1, 8, STRIPE);

  gRect(x + 30, y + 4, 1, 5, gC(60, 56, 50));
  gPx(x + 30, y + 5, AMBER_HOT);
  gPx(x + 30, y + 7, AMBER_HOT);
  for (int i = 1; i < 10; i++) gPxa(x + 31 + i, y + 6, AMBER, 0.32f - i * 0.031f);

  gText("10", x + 9, y + 1, 1, gC(30, 28, 26));

  for (int w = 0; w < 2; w++) {
    float wx = x + (w == 0 ? 7 : 24);
    gCircle(wx, y + 12, 4, TIRE);
    gCircle(wx, y + 12, 1.8f, gC(162, 162, 166));
    float a = t * 8 + (w == 0 ? 0 : 1.2f);
    gPx(wx + cosf(a) * 2.8f, y + 12 + sinf(a) * 2.8f, gC(92, 92, 96));
  }
}

// =====================================================================
// Scenes
// =====================================================================

static void gateLeaf(float x0, float x1, int doorTop) {
  gSetClip(10, doorTop, 53, 31);
  gRect(x0, doorTop, x1 - x0, 32 - doorTop, WOOD);
  for (float x = x0; x < x1; x += 4) {
    gRect(x, doorTop, 1, 32 - doorTop, WOOD_DK);
    gRect(x + 1, doorTop, 1, 32 - doorTop, WOOD_LT);
  }
  for (int b = 0; b < 2; b++) {
    float by = (b == 0) ? doorTop + 2 : 26;
    gRect(x0, by, x1 - x0, 2, gC(62, 58, 60));
    for (float x = x0 + 1; x < x1; x += 5) gPx(x, by + 1, gC(128, 124, 126));
  }
  gResetClip();
}

static void sceneGate(float t) {
  float openF = gSmooth((t - 2.2f) / 2.6f);
  float gap = openF * 21;

  nightSky(t);
  for (int y = 10; y < FB_H; y++)
    for (int x = 10; x < 54; x++) {
      float d = hypotf((x - 32) * 0.7f, y - 30);
      gPxa(x, y, JP_ORANGE, gClamp((1 - d / 26) * 0.85f * openF, 0, 1));
    }

  gSetClip(10, 12, 53, 31);
  for (int x = 10; x < 54; x++) {
    int top = 19 + (int)floorf(gHash2(x, 5) * 3 + gHash2((int)floorf(x / 6.0f), 13) * 4);
    for (int y = top; y < 32; y++) gPx(x, y, gC(7, 20, 12));
  }
  for (int y = 12; y < 32; y++)
    for (int x = 10; x < 54; x++) {
      float d = hypotf((x - 32) * 0.7f, y - 31);
      gPxa(x, y, JP_ORANGE, gClamp((1 - d / 24) * 0.55f * openF, 0, 1));
    }
  if (openF > 0.62f) {
    miniRex(15, 13, SIL, gC(20, 14, 12));
    float lid = (jsMod(t, 2.8f) < 2.5f) ? 1.0f : 0.15f;
    gPx(18, 17, gMix(SIL, AMBER_HOT, lid));
  }
  gResetClip();

  const int doorTop = 12;
  gateLeaf(10 - gap, 32 - gap, doorTop);
  gateLeaf(32 + gap, 54 + gap, doorTop);

  for (int s = 0; s < 2; s++) {
    float side = (s == 0) ? -1.0f : 1.0f;
    float cx = 32 + side * (gap + 0.5f);
    gSetClip(side < 0 ? 10 : 32, doorTop, side < 0 ? 31 : 53, 31);
    gCircle(cx - side * 0.5f, 21, 7, AMBER_DIM);
    gCircle(cx - side * 0.5f, 21, 5.6f, WOOD_DK);
    gQuad(cx - 4, 20, cx + 1, 18.6f, cx + 3, 20.4f, cx - 3.4f, 21.4f, AMBER);
    gQuad(cx - 3.6f, 21.6f, cx + 2.6f, 21, cx + 2.6f, 22.4f, cx - 3.2f, 22.8f, AMBER);
    gPx(cx + 0.6f, 19.8f, WOOD_DK);
  }
  gResetClip();

  for (int p = 0; p < 2; p++) {
    float px0 = (p == 0) ? 0 : 54, px1 = (p == 0) ? 10 : 64;
    gRect(px0, 4, px1 - px0, 28, STONE);
    for (int y = 6; y < 32; y += 5) gRect(px0, y, px1 - px0, 1, STONE_DK);
    for (int y = 6; y < 32; y += 10) gRect(px0 + (px1 - px0) / 2, y, 1, 5, STONE_DK);
    gRect(px0 - 1, 3, px1 - px0 + 2, 2, STONE_LT);
  }
  // Banner in the park's own colours rather than the gate's timber: black
  // field, red rules, yellow lettering carrying a red drop shadow.
  gRect(9, 3, 46, 10, BLACK);
  gRect(9, 3, 46, 1, JP_RED);
  gRect(9, 12, 46, 1, JP_RED);
  // Shadow offset sideways only. A diagonal drop shadow on a 5x7 outline
  // font lands inside the letter counters and reads as red on both sides.
  float nx = floorf((FB_W - gTextWidth("NOLAN", 1)) / 2.0f + 0.5f);
  gText("NOLAN", nx + 1, 4, 1, JP_RED);
  gText("NOLAN", nx, 4, 1, gMix(gC(255, 206, 48), gC(255, 238, 150), 0.35f + 0.35f * sinf(t * 2)));

  torch(4.5f, 4, t, 3);
  torch(58.5f, 4, t, 17);
}

static void sceneWelcome(float t) {
  gClear(gC(5, 14, 9));
  treeline(20, JUNGLE, 4);
  treeline(26, gC(4, 14, 9), 21);
  for (int y = 0; y < FB_H; y++)
    for (int x = 0; x < FB_W; x++) {
      float d = fminf(hypotf(x - 2, y - 8), hypotf(x - 61, y - 8));
      gPxa(x, y, JP_ORANGE, gClamp((1 - d / 34) * 0.30f, 0, 1));
    }

  static const char *LINES[3] = { "WELCOME TO", "NOLAN PARK", "EST 2020" };
  static const int LINE_Y[3] = { 2, 12, 23 };
  for (int i = 0; i < 3; i++) {
    float appear = gClamp((t - 0.35f - i * 0.75f) / 0.5f, 0, 1);
    if (appear <= 0) continue;
    int len = strlen(LINES[i]);
    int n = (int)ceilf(appear * len);
    char shown[16];
    strncpy(shown, LINES[i], n);
    shown[n] = 0;
    float x = floorf((FB_W - gTextWidth(LINES[i], 1)) / 2.0f + 0.5f);
    uint32_t col = (i == 1) ? gMix(AMBER, AMBER_HOT, 0.5f + 0.5f * sinf(t * 3)) : AMBER;
    gText(shown, x, LINE_Y[i], 1, col);
  }
  float uw = gClamp((t - 1.6f) / 0.6f, 0, 1) * 40;
  if (uw > 0) gRect(32 - uw / 2, 20, uw, 1, JP_RED);
}

static void sceneTrex(float t) {
  float flash = fmaxf(fmaxf(gPulse(t, 3.9f, 4.12f), gPulse(t, 4.28f, 4.42f)), gPulse(t, 7.4f, 7.58f));
  sunsetSky(27);
  gCircle(50, 25, 8, gMix(gC(255, 200, 80), WHITE, flash * 0.6f));
  gCircle(50, 25, 5.5f, gMix(gC(255, 226, 150), WHITE, flash * 0.6f));
  for (int x = 0; x < FB_W; x++) {
    int top = 25 + (int)floorf(gHash2(x, 11) * 1.9f);
    for (int y = top; y < 27; y++) gPx(x, y, gC(9, 26, 15));
  }
  gRect(0, 27, FB_W, 5, gC(5, 14, 9));

  float roar = fmaxf(gPulse(t, 2.1f, 3.6f), gPulse(t, 5.6f, 7.0f));
  float jaw = roar * 0.62f;
  float lift = roar * 0.22f;
  float shake = (roar > 0.4f) ? floorf(sinf(t * 40) + 0.5f) : 0;
  float bob = sinf(t * 2.2f) * 0.7f;

  Pt mouth = trex(3 + shake, bob + shake * 0.5f, jaw, lift, t * 2.2f);

  if (roar > 0.15f) {
    for (int k = 0; k < 3; k++) {
      float rr = 3 + jsMod(t * 18 + k * 4.2f, 12.0f);
      float aa = gClamp((1 - rr / 15) * roar, 0, 1);
      for (float d = -0.85f; d <= 0.85f; d += 0.06f)
        gPxa(mouth.x - cosf(d) * rr, mouth.y - sinf(d) * rr * 0.8f, AMBER_HOT, aa);
    }
  }

  if (t > 2.3f && t < 6.0f) {
    float f = t - 2.3f;
    for (int i = 0; i < 5; i++) {
      float bx = 44 + i * 3.5f + f * (5 + gHash(i * 9) * 5);
      float by = 12 - f * (2.2f + gHash(i * 21) * 2.4f) - i * 0.8f;
      if (bx > FB_W + 4 || by < -4) continue;
      float flap = (sinf(f * 14 + i * 2) > 0) ? 1.0f : 0.0f;
      gPx(bx, by, SIL); gPx(bx - 1, by - flap, SIL); gPx(bx + 1, by - flap, SIL);
    }
  }

  if (flash > 0.02f)
    for (int y = 0; y < FB_H; y++)
      for (int x = 0; x < FB_W; x++) gPxa(x, y, WHITE, flash * 0.7f);
}

static void sceneJeep(float t) {
  sunsetSky(24);
  for (int x = 0; x < FB_W; x++) {
    int top = 16 + (int)floorf(gHash2(x, 7) * 2 + gHash2((int)floorf(x / 7.0f), 23) * 3);
    for (int y = top; y < 24; y++) gPx(x, y, gC(8, 24, 14));
  }
  for (int y = 24; y < FB_H; y++) {
    uint32_t c = gMix(gC(102, 70, 40), gC(58, 38, 22), (y - 24) / 8.0f);
    for (int x = 0; x < FB_W; x++) gPx(x, y, c);
  }
  for (int i = 0; i < 34; i++)
    gPxa(floorf(gHash(i * 3 + 1) * FB_W), 24 + floorf(gHash(i * 5 + 2) * 8), gC(138, 100, 58), 0.5f);

  float shake = 0;
  for (int i = 0; i < 4; i++) {
    float st = 2.4f + i * 1.05f;
    if (t < st) continue;
    float f = gClamp((t - st) / 0.28f, 0, 1);
    footprint(4 + i * 15, 29, f);
    if (f < 0.5f) shake = 1;
  }

  float x = -36 + jsMod(t * 22, 102.0f);
  float bounce = (fabsf(sinf(t * 10)) > 0.7f) ? 1.0f : 0.0f;
  if (x < 68) jeep(x, 11 + bounce + shake, t);
}

static void sceneBlue(float t) {
  gClear(gC(3, 10, 7));
  treeline(8, gC(6, 20, 12), 55);
  for (int i = 0; i < 3; i++) {
    float ex = 46 + i * 6, ey = 4 + i * 3;
    if (sinf(t * 1.3f + i * 2.1f) > -0.2f) {
      gPxa(ex, ey, gC(210, 150, 30), 0.75f);
      gPxa(ex + 2, ey, gC(210, 150, 30), 0.75f);
    }
  }
  float blink = (jsMod(t, 3.4f) > 3.15f) ? 1.0f : 0.0f;
  float snap = fmaxf(gPulse(t, 2.4f, 3.0f), gPulse(t, 5.1f, 5.6f));
  float bob = sinf(t * 1.6f) * 1.2f;
  raptorHead(7, 7 + bob, blink, snap * 0.32f, gC(88, 124, 80), gC(46, 116, 210));

  for (int i = 0; i < 6; i++) {
    float ph = jsMod(t * 0.55f + i * 0.16f, 1.0f);
    gPxa(56 + ph * 7, 13 + bob + sinf(ph * 6 + i) * 2 - ph * 2, gC(150, 175, 190), (1 - ph) * 0.30f);
  }
}

static void sceneEgg(float t) {
  gClear(gC(8, 5, 12));
  for (int y = 0; y < FB_H; y++)
    for (int x = 0; x < FB_W; x++) {
      float d = hypotf((x - 32) * 0.8f, y - 20);
      gPxa(x, y, JP_ORANGE, gClamp((1 - d / 30) * 0.5f, 0, 1));
    }
  gRect(0, 29, FB_W, 3, gC(52, 34, 18));
  for (int x = 0; x < FB_W; x += 3) gPx(x, 29, gC(76, 52, 28));

  float wob = (t < 3.0f) ? sinf(t * 9) * (0.6f + t * 0.35f) : 0;
  float capF = gSmooth((t - 3.0f) / 0.7f);
  float headF = gSmooth((t - 3.4f) / 1.3f);

  float ex = 32 + wob, ey = 19;
  gEll(ex, ey, 8.5f, 11, gC(238, 226, 198));
  gEll(ex - 2.5f, ey - 3, 4.5f, 6, gC(250, 242, 222));
  for (int i = 0; i < 22; i++) {
    float sx = ex - 7 + gHash(i * 13) * 14;
    float sy = ey - 9 + gHash(i * 29) * 18;
    if (hypotf((sx - ex) / 8.5f, (sy - ey) / 11) < 0.92f) gPxa(sx, sy, gC(196, 176, 142), 0.6f);
  }

  if (headF > 0) {
    float hy = ey - 6 - headF * 7;
    gEll(ex, hy, 4.2f, 3.6f, gC(112, 138, 92));
    gQuad(ex + 2, hy - 1, ex + 8, hy + 0.6f, ex + 8, hy + 2.4f, ex + 2, hy + 2.6f, gC(112, 138, 92));
    gLine(ex - 3, hy - 3, ex + 4, hy - 4.2f, gC(52, 120, 206), 2, 1.6f);
    gCircle(ex + 1.4f, hy - 0.6f, 1.6f, gC(255, 214, 60));
    gPx(ex + 1.4f, hy - 0.6f, gC(16, 10, 6));
    gPx(ex + 2.1f, hy - 1.3f, WHITE);
    gPx(ex + 7, hy + 1, gC(60, 74, 48));
    float ch = gPulse(jsMod(t, 1.6f), 0.1f, 0.45f);
    if (ch > 0.1f)
      for (int k = 1; k <= 2; k++) {
        float rr = 3 + k * 2.2f;
        for (float d = -0.6f; d <= 0.6f; d += 0.12f)
          gPxa(ex + 8 + cosf(d) * rr, hy + 1 + sinf(d) * rr, AMBER_HOT, ch * 0.7f);
      }
  }

  if (t > 1.4f) {
    float cf = gClamp((t - 1.4f) / 1.6f, 0, 1);
    for (int i = 0; i < 7; i++) {
      if (i / 7.0f > cf) break;
      float a = -2.4f + i * 0.34f;
      float x0 = ex + cosf(a) * 7, y0 = ey + sinf(a) * 9;
      gLine(x0, y0, x0 + cosf(a + 1.4f) * 2.4f, y0 + sinf(a + 1.4f) * 2.4f, gC(120, 104, 78), 1);
    }
  }
  if (capF > 0) {
    float cx2 = ex + 4 + capF * 12;
    float cy2 = ey - 9 - sinf(capF * (float)M_PI) * 7 + capF * 10;
    Pt r1 = rotp(cx2 - 4, cy2, cx2, cy2, capF * 2.4f);
    Pt r2 = rotp(cx2 + 4, cy2, cx2, cy2, capF * 2.4f);
    gLine(r1.x, r1.y, r2.x, r2.y, gC(238, 226, 198), 3, 3);
  }

  for (int i = 0; i < 10; i++) {
    float ph = jsMod(t * 0.4f + gHash(i * 7), 1.0f);
    gPxa(12 + gHash(i * 11) * 40, 28 - ph * 22, AMBER_HOT, (1 - ph) * 0.45f);
  }
}

static void sceneLogo(float t) {
  gClear(gC(3, 3, 6));
  float grow = gSmooth(t / 0.9f);
  float RX = 15 * grow, RY = 11.5f * grow;
  if (RX < 2) return;

  gEll(32, 12, RX, RY, BLACK);
  for (int y = 0; y <= 26; y++)
    for (int x = 16; x <= 48; x++) {
      if (hypotf((x - 32) / (RX - 1.8f), (y - 12) / (RY - 1.8f)) > 1) continue;
      gPx(x, y, gMix(gC(248, 150, 34), gC(214, 58, 26), y / 22.0f));
    }

  if (grow > 0.9f) miniRex(19, 2, BLACK, gC(86, 20, 16));

  gRect(1, 25, 62, 7, BLACK);
  gRect(1, 25, 62, 1, gC(72, 56, 34));
  gTextCentered("NOLAN PARK", 25, 1, AMBER);

  if (t > 1.2f) {
    float sw = jsMod((t - 1.2f) * 32, 112.0f);
    for (int y = 0; y < 25; y++) {
      float x = sw - 24 + y * 0.7f;
      for (int k = 0; k < 3; k++)
        if (hypotf((x + k - 32) / (RX - 2), (y - 12) / (RY - 2)) < 1)
          gPxa(x + k, y, WHITE, 0.20f - k * 0.055f);
    }
  }
}

static void sceneBirthday(float t) {
  gClear(gC(6, 4, 10));

  static const uint32_t HUES[5] = { AMBER, 0x5ADC8C, 0x5AAAFF, JP_RED, 0xFF78C8 };
  for (int bidx = 0; bidx < 14; bidx++) {
    float age = t - bidx * 0.8f;
    if (age < 0 || age > 1.5f) continue;
    float bx = 8 + gHash(bidx * 17 + 3) * 48;
    float by = 5 + gHash(bidx * 29 + 11) * 18;
    uint32_t hue = HUES[bidx % 5];
    for (int p = 0; p < 12; p++) {
      float a = (p / 12.0f) * (float)M_PI * 2 + gHash(bidx * 7) * 6;
      float sp = 7 + gHash(bidx * 13 + p) * 5;
      float d = age * sp;
      gPxa(bx + cosf(a) * d, by + sinf(a) * d * 0.85f + age * age * 6,
           hue, gClamp(1 - age / 1.5f, 0, 1) * 0.95f);
    }
  }

  if (t < 4.0f) {
    const char *s = "HAPPY BIRTHDAY";
    float w = gTextWidth(s, 2);
    gText(s, FB_W - jsMod(t * 30, w + FB_W), 9, 2, AMBER);
  } else {
    float f = t - 4.0f;
    uint32_t c1 = gMix(AMBER, AMBER_HOT, 0.5f + 0.5f * sinf(f * 4));
    uint32_t c2 = gMix(gC(90, 220, 140), WHITE, 0.5f + 0.5f * sinf(f * 4 + 1.6f));
    gTextCentered("NOLAN", 2, 2, c1);
    gTextCentered("IS 6!", 17, 2, c2);
    for (int i = 0; i < 2; i++) {
      float rx = jsMod(f * 17 + i * 33, 78.0f) - 8;
      float ry = 31 - fabsf(sinf(f * 9 + i)) * 1;
      gEll(rx, ry - 2, 2.4f, 1.4f, gC(70, 190, 120));
      gLine(rx + 2, ry - 2.5f, rx + 5, ry - 3.4f, gC(70, 190, 120), 1.4f, 1);
      gLine(rx - 2, ry - 2, rx - 5, ry - 3.6f, gC(70, 190, 120), 1.4f, 1);
      gPx(rx + 4.6f, ry - 3.6f, gC(255, 220, 90));
    }
  }

  uint32_t bulb = gMix(AMBER, AMBER_HOT, 0.5f + 0.5f * sinf(t * 6));
  for (int x = 1; x < FB_W - 1; x += 5) { gPx(x, 0, bulb); gPx(x, FB_H - 1, bulb); }
  for (int y = 1; y < FB_H - 2; y += 5) { gPx(0, y, bulb); gPx(FB_W - 1, y, bulb); }
}

const SceneDef SCENES[] = {
  { "gate",     "The Gates",    9.0f,  sceneGate },
  { "welcome",  "Welcome Sign", 6.0f,  sceneWelcome },
  { "trex",     "T-Rex",        9.0f,  sceneTrex },
  { "jeep",     "Jeep 10",      7.0f,  sceneJeep },
  { "blue",     "Blue",         7.0f,  sceneBlue },
  { "egg",      "Hatching Egg", 8.0f,  sceneEgg },
  { "logo",     "Park Emblem",  6.0f,  sceneLogo },
  { "birthday", "Nolan Is 6",   11.0f, sceneBirthday },
};
const int SCENE_COUNT = sizeof(SCENES) / sizeof(SCENES[0]);

int sceneIndexByKey(const char *key) {
  for (int i = 0; i < SCENE_COUNT; i++)
    if (strcmp(SCENES[i].key, key) == 0) return i;
  return -1;
}
