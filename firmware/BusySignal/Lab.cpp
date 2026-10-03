#include "Lab.h"
#include "Gfx.h"
#include <math.h>

// ---- Brand palette (npts-branding: lockup, wallpaper, service label) ----
static const uint32_t CYAN       = 0x00E5FF;
static const uint32_t MAGENTA    = 0xFF2D9B;
static const uint32_t MAG_ECHO   = 0x600A3C;
static const uint32_t LAMP_BUSY  = 0xFF1616;
static const uint32_t LAMP_OPEN  = 0x14EB50;
static const uint32_t BLACK      = 0x000000;

static const uint32_t JET_BODY   = 0x3A4A68;
static const uint32_t JET_RIM    = 0x78DCFF;
static const uint32_t WING_TOP   = 0x54688C;
static const uint32_t WING_BELLY = 0x1E263A;
static const uint32_t FIN        = 0x465A7E;
static const uint32_t CANOPY     = 0xAAE8FF;
static const uint32_t FLAME_HOT  = 0xFFECAA;
static const uint32_t FLAME      = 0xFF7828;
static const uint32_t WAKE_FAR   = 0x781E5A;
static const uint32_t TEXT_HOT   = 0xFFF0C8;
static const uint32_t TEXT       = 0xFF963A;

static const uint32_t NO_ECHO    = 0xFFFFFFFF;

// The 16-band sunset ramp the wallpaper, badges and labels are built on.
struct Stop { float at; float r, g, b; };
static const Stop STOPS[] = {
  { 0.00f, 10, 22, 48 }, { 0.28f, 13, 58, 86 }, { 0.52f, 92, 34, 64 },
  { 0.74f, 196, 64, 32 }, { 1.00f, 255, 150, 58 },
};
static const int STOP_COUNT = sizeof(STOPS) / sizeof(STOPS[0]);

static uint32_t ramp(float f) {
  for (int i = 0; i < STOP_COUNT - 1; i++) {
    const Stop &a = STOPS[i], &b = STOPS[i + 1];
    if (f >= a.at && f <= b.at) {
      float k = (f - a.at) / (b.at - a.at);
      return gC(a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k);
    }
  }
  const Stop &l = STOPS[STOP_COUNT - 1];
  return gC(l.r, l.g, l.b);
}

// The ramp's navy end is nearly black on an LED, so the rules take each
// band's hue at a common brightness instead of its printed value.
static uint32_t band(int x) {
  uint32_t c = ramp((floorf(x / 4.0f) + 0.5f) / 16);
  int m = max(gCr(c), max(gCg(c), gCb(c)));
  return gScale(c, 210.0f / m);
}

// ---- Logo lettering ----
// Hand-cut 7-row glyphs after the NEON PULSE lockup: square shoulders,
// notched corners and the stepped N. Only the letters the sign uses.
struct Glyph { char ch; const char *rows[7]; };
static const Glyph GLYPHS[] = {
  { 'N', { "#...#", "##..#", "##..#", "#.#.#", "#..##", "#..##", "#...#" } },
  { 'E', { "####", "#...", "#...", "###.", "#...", "#...", "####" } },
  { 'O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
  { 'P', { "###.", "#..#", "#..#", "###.", "#...", "#...", "#..." } },
  { 'U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
  { 'L', { "#...", "#...", "#...", "#...", "#...", "#...", "####" } },
  { 'S', { "####", "#...", "#...", "####", "...#", "...#", "####" } },
  { 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
  { 'B', { "###.", "#..#", "#..#", "###.", "#..#", "#..#", "###." } },
};

static const Glyph *findGlyph(char ch) {
  for (unsigned i = 0; i < sizeof(GLYPHS) / sizeof(GLYPHS[0]); i++)
    if (GLYPHS[i].ch == ch) return &GLYPHS[i];
  return nullptr;
}

static int glyphWidth(char ch) { return (int)strlen(findGlyph(ch)->rows[0]); }

static void glyph(char ch, float x, float y, uint32_t c) {
  const Glyph *g = findGlyph(ch);
  for (int r = 0; r < 7; r++)
    for (int k = 0; g->rows[r][k]; k++)
      if (g->rows[r][k] == '#') gPx(x + k, y + r, c);
}

static int wordWidth(const char *s, int gap, int space) {
  int w = 0;
  int n = (int)strlen(s);
  for (int i = 0; i < n; i++) {
    w += s[i] == ' ' ? space : glyphWidth(s[i]);
    if (i < n - 1) w += gap;
  }
  return w;
}

// The lockup's echo: the same letters again one pixel down-right in the
// partner colour, drawn first so the face sits on top of it.
static void word(const char *s, int x, int y, int gap, int space, uint32_t face, uint32_t echo) {
  for (int pass = 0; pass < 2; pass++) {
    int cx = x;
    for (int i = 0; s[i]; i++) {
      if (s[i] == ' ') { cx += space + gap; continue; }
      if (pass == 1) glyph(s[i], cx, y, face);
      else if (echo != NO_ECHO) glyph(s[i], cx + 1, y + 1, echo);
      cx += glyphWidth(s[i]) + gap;
    }
  }
}

static inline int jsRoundI(float v) { return (int)floorf(v + 0.5f); }

static void logo() {
  const char *top = "NEON PULSE";
  word(top, jsRoundI((FB_W - wordWidth(top, 1, 2)) / 2.0f), 1, 1, 2, CYAN, MAG_ECHO);
  // LAB goes without the echo: a cyan echo under magenta turned the
  // counters to mud, and the row it needs belongs to the jet.
  int lw = wordWidth("LAB", 2, 0);
  int lx = jsRoundI((FB_W - lw) / 2.0f);
  word("LAB", lx, 9, 2, 0, MAGENTA, NO_ECHO);
  // Ramp-coloured rules either side of LAB, doubled like the lockup's
  // outline: a lit tube with a dimmer echo under it.
  for (int x = 7; x < lx - 3; x++) { gPx(x, 11, band(x)); gPx(x, 13, gScale(band(x), 0.45f)); }
  for (int x = lx + lw + 4; x <= 56; x++) { gPx(x, 11, band(x)); gPx(x, 13, gScale(band(x), 0.45f)); }
}

// ---- Corner lamps ----
static const float LAMPS[4][2] = { { 2, 2 }, { 61, 2 }, { 2, 29 }, { 61, 29 } };
static const float LAMP_BREATH = 3.0f;

static void lamps(float t, bool busy) {
  uint32_t base = busy ? LAMP_BUSY : LAMP_OPEN;
  // A slow breath so they read as lit lamps rather than painted dots. Three
  // seconds, so the idle pass is a whole number of breaths and the panel
  // can wrap its clock at a pass boundary without a visible step.
  float b = 0.80f + 0.20f * sinf((t * 2 * (float)M_PI) / LAMP_BREATH);
  for (int i = 0; i < 4; i++) {
    float x = LAMPS[i][0], y = LAMPS[i][1];
    gEll(x, y, 3.4f, 3.4f, BLACK);
    gEll(x, y, 2.4f, 2.4f, gScale(base, 0.45f * b));
    gEll(x, y, 1.5f, 1.5f, gScale(base, b));
    gPx(x - 1, y - 1, gMix(gScale(base, b), gC(255, 255, 255), 0.45f));
  }
}

// ---- The jet ----
// Model space: x runs nose (0) to tail (JET_LEN), y is up, z is toward
// the viewer. The fuselage is treated as round, so its side profile never
// changes; canopy, wings, fin and tailplanes are 3D and turn with a roll.
static const float JET_LEN = 20;
static const float FUSE[][2] = {
  { 0, 0.3f }, { 2.5f, -0.9f }, { 6, -1.4f }, { 15, -1.4f }, { 19.6f, -1.0f },
  { 19.6f, 1.0f }, { 14, 1.4f }, { 4, 1.3f }, { 1.5f, 0.9f },
};
static const int FUSE_N = sizeof(FUSE) / sizeof(FUSE[0]);

struct Part { bool fin; float pts[4][3]; };
static const Part PARTS[] = {
  { false, { { 7, -0.3f, 0.6f }, { 15, -0.3f, 5.4f }, { 16.8f, -0.3f, 5.4f }, { 17.6f, -0.3f, 0.6f } } },
  { false, { { 7, -0.3f, -0.6f }, { 15, -0.3f, -5.4f }, { 16.8f, -0.3f, -5.4f }, { 17.6f, -0.3f, -0.6f } } },
  { false, { { 16, 0, 0.6f }, { 18.8f, 0, 2.6f }, { 19.8f, 0, 2.6f }, { 19.8f, 0, 0.6f } } },
  { false, { { 16, 0, -0.6f }, { 18.8f, 0, -2.6f }, { 19.8f, 0, -2.6f }, { 19.8f, 0, -0.6f } } },
  { true,  { { 13.6f, 1.0f, 0 }, { 17.4f, 4.8f, 0 }, { 19.4f, 4.8f, 0 }, { 19.6f, 1.0f, 0 } } },
};
static const int PART_N = sizeof(PARTS) / sizeof(PARTS[0]);

// Resting roll: banked a little toward the viewer so the near wing shows
// below the fuselage, the way the wallpaper jet is drawn.
static const float ROLL0 = 0.42f;

struct Item { float z; bool canopy; bool fin; float scr[4][2]; uint32_t col; };

static void drawItem(const Item &it, float jx, float jy, float canY, float cs) {
  if (it.canopy) {
    gEll(jx + 6.2f, jy - canY, 2.2f, 0.8f, CANOPY);
    return;
  }
  const float (*s)[2] = it.scr;
  gQuad(s[0][0], s[0][1], s[1][0], s[1][1], s[2][0], s[2][1], s[3][0], s[3][1], it.col);
  // The wallpaper jet's fin has a lit leading edge; it fades out as the
  // fin turns edge-on.
  if (it.fin && fabsf(cs) > 0.35f) gLine(s[0][0], s[0][1], s[1][0], s[1][1], JET_RIM, 1);
}

static void jet(float jx, float jy, float roll) {
  float cs = cosf(roll), sn = sinf(roll);
  // Project every part, then paint far-to-near around the fuselage.
  Item items[PART_N + 1];
  int n = 0;
  for (int i = 0; i < PART_N; i++) {
    const Part &p = PARTS[i];
    Item &it = items[n++];
    float zsum = 0;
    for (int k = 0; k < 4; k++) {
      float x = p.pts[k][0], y = p.pts[k][1], z = p.pts[k][2];
      float y2 = y * cs - z * sn, z2 = y * sn + z * cs;
      it.scr[k][0] = jx + x;
      it.scr[k][1] = jy - y2;
      zsum += z2;
    }
    // Wings face +y, the fin faces +z. Which side we see sets the colour,
    // how squarely we see it sets the brightness.
    if (!p.fin) it.col = gScale(sn >= 0 ? WING_TOP : WING_BELLY, 0.55f + 0.45f * fabsf(sn));
    else it.col = gScale(FIN, 0.6f + 0.4f * fabsf(cs));
    it.z = zsum / 4;
    it.canopy = false;
    it.fin = p.fin;
  }
  float canY = 1.5f * cs, canZ = 1.5f * sn;
  items[n].z = canZ;
  items[n].canopy = true;
  items[n].fin = false;
  n++;
  // Insertion sort: stable, like the simulator's Array.sort, so equal
  // depths paint in the same order on both.
  for (int i = 1; i < n; i++) {
    Item key = items[i];
    int j = i - 1;
    while (j >= 0 && items[j].z > key.z) { items[j + 1] = items[j]; j--; }
    items[j + 1] = key;
  }

  int i = 0;
  for (; i < n && items[i].z < 0; i++) drawItem(items[i], jx, jy, canY, cs);
  for (int k = 1; k < FUSE_N - 1; k++)
    gTri(jx + FUSE[0][0], jy + FUSE[0][1], jx + FUSE[k][0], jy + FUSE[k][1],
         jx + FUSE[k + 1][0], jy + FUSE[k + 1][1], JET_BODY);
  // Light comes from above, so the rim is the top edge of the profile
  // whichever way up the jet is.
  for (int x = 3; x <= 18; x++) gPx(jx + x, jy - 1.4f, JET_RIM);
  gPx(jx + 1.6f, jy - 0.6f, JET_RIM);
  gPx(jx + 0.4f, jy + 0.1f, JET_RIM);
  for (; i < n; i++) drawItem(items[i], jx, jy, canY, cs);
}

static void flame(float nx, float ny, float t) {
  float f = gHash2((int)floorf(t * 24), 7);
  float len = 1.6f + f * 1.8f;
  gEll(nx + len * 0.5f, ny, len * 0.6f + 0.4f, 0.9f, FLAME);
  gPx(nx + 0.5f, ny, FLAME_HOT);
}

// ---- Flight paths ----
// Idle pass: cruise in from the right, ease off and roll at centre,
// cruise out to the left, then a quiet gap with just the logo.
static const float AXIS_Y = 23;
static const float VC = 26;              // cruise, px/s
static const float VR = 7;               // slowest point of the roll
static const float SLOW0 = 1.15f, SLOW_D = 2.4f;
static const float ROLL_A = 1.55f, ROLL_D = 1.6f;
static const float HELIX_R = 2.0f;
static const float IDLE_CYCLE = 9.0f;

// Distance lost to the slowdown: integral of (VC-VR)*sin^2 over the window.
static float slowLoss(float t) {
  float u = gClamp(t - SLOW0, 0, SLOW_D);
  return (VC - VR) * (u / 2 - (SLOW_D / (4 * (float)M_PI)) * sinf((2 * (float)M_PI * u) / SLOW_D));
}

struct Pose { float x, y, roll; };

static Pose idlePose(float t) {
  float x = FB_W - VC * t + slowLoss(t);
  float r = gSmooth((t - ROLL_A) / ROLL_D);
  float phi = r * 2 * (float)M_PI;
  // Climb over the top of the roll, inverted at the apex: a barrel roll
  // seen side-on rather than a flat aileron roll.
  float y = AXIS_Y - HELIX_R * (1 - cosf(phi));
  return { x, y, ROLL0 + phi };
}

static void idle(float t) {
  float tt = fmodf(t, IDLE_CYCLE);
  Pose p = idlePose(tt);
  if (p.x > FB_W + 4 || p.x < -JET_LEN - 30) return;
  // Exhaust trail: where the nozzle has been, so the corkscrew of the
  // roll stays drawn in the air behind it.
  for (int k = 26; k >= 1; k--) {
    float tp = tt - k * 0.03f;
    if (tp < 0) continue;
    Pose q = idlePose(tp);
    float f = k / 26.0f;
    gPxa(q.x + JET_LEN - 0.4f, q.y, gMix(FLAME, WAKE_FAR, f), (1 - f) * 0.85f);
  }
  flame(p.x + JET_LEN - 0.4f, p.y, t);
  jet(p.x, p.y, p.roll);
}

// Message pass: straight and level at a reading pace, the text riding in
// the exhaust. Letters are hottest right behind the nozzle and cool to
// the settled colour a couple of characters back.
static const float VM = 20;
static const float MSG_GAP = 5;
static const float MSG_PAUSE = 1.0f;

float labCycle(const char *msg) {
  if (!msg || !*msg) return IDLE_CYCLE;
  return (FB_W + JET_LEN + MSG_GAP + gTextWidth(msg, 1) + 2) / VM + MSG_PAUSE;
}

static void message(float t, const char *msg) {
  float tt = fmodf(t, labCycle(msg));
  float x = FB_W - VM * tt;
  float y = AXIS_Y;
  float nx = x + JET_LEN - 0.4f;
  float tx = nx + MSG_GAP;
  for (int k = 0; k < 4; k++) {
    float sx = nx + 1 + k;
    gPxa(sx, y, gMix(FLAME, WAKE_FAR, k / 4.0f), 0.7f - k * 0.15f);
  }
  char one[2] = { 0, 0 };
  for (int i = 0; msg[i]; i++) {
    float cx = tx + i * 6;
    if (cx > FB_W || cx < -6) continue;
    float heat = gClamp(1 - (cx - tx) / 18, 0, 1);
    one[0] = msg[i];
    gText(one, cx, y - 3, 1, gMix(TEXT, TEXT_HOT, heat * heat));
  }
  flame(nx, y, t);
  jet(x, y, ROLL0);
}

void labFrame(float t, bool busy, const char *msg) {
  logo();
  gSetClip(0, 16, FB_W - 1, FB_H - 1);
  if (msg && *msg) message(t, msg);
  else idle(t);
  gResetClip();
  lamps(t, busy);
}
