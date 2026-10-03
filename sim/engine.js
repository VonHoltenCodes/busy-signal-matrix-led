/*
 * Busy Signal scene engine - shared by the browser simulator and the
 * headless renderer.
 *
 * Every primitive here is written to be portable 1:1 into the firmware:
 * pure integer/float math over a pixel buffer, no Adafruit_GFX calls. The
 * firmware keeps its own copy of these same functions so a scene drawn
 * here lands on the panel pixel-for-pixel.
 */
(function (global) {
  const W = 64, H = 32;

  const buf = new Uint8Array(W * H * 3);
  let clipX0 = 0, clipY0 = 0, clipX1 = W - 1, clipY1 = H - 1;

  // Clamp, don't mask: scaleC() routinely pushes a channel past 255 and a
  // bitwise & wraps it around into a completely different hue.
  function cl8(v) { v = Math.round(v); return v < 0 ? 0 : v > 255 ? 255 : v; }
  function C(r, g, b) { return (cl8(r) << 16) | (cl8(g) << 8) | cl8(b); }
  function cr(c) { return (c >> 16) & 255; }
  function cg(c) { return (c >> 8) & 255; }
  function cb(c) { return c & 255; }

  function setClip(x0, y0, x1, y1) { clipX0 = x0; clipY0 = y0; clipX1 = x1; clipY1 = y1; }
  function resetClip() { setClip(0, 0, W - 1, H - 1); }

  function px(x, y, c) {
    x = Math.round(x); y = Math.round(y);
    if (x < clipX0 || x > clipX1 || y < clipY0 || y > clipY1) return;
    const i = (y * W + x) * 3;
    buf[i] = cr(c); buf[i + 1] = cg(c); buf[i + 2] = cb(c);
  }

  // Alpha blend onto whatever is already there - used for glows, fades and
  // anti-flicker. a is 0..1.
  function pxa(x, y, c, a) {
    x = Math.round(x); y = Math.round(y);
    if (x < clipX0 || x > clipX1 || y < clipY0 || y > clipY1) return;
    if (a <= 0) return;
    if (a > 1) a = 1;
    const i = (y * W + x) * 3;
    buf[i] = buf[i] + (cr(c) - buf[i]) * a;
    buf[i + 1] = buf[i + 1] + (cg(c) - buf[i + 1]) * a;
    buf[i + 2] = buf[i + 2] + (cb(c) - buf[i + 2]) * a;
  }

  function getPx(x, y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return 0;
    const i = (y * W + x) * 3;
    return C(buf[i], buf[i + 1], buf[i + 2]);
  }

  function clear(c) {
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) px(x, y, c);
  }

  function rect(x, y, w, h, c) {
    for (let yy = Math.round(y); yy < Math.round(y) + Math.round(h); yy++)
      for (let xx = Math.round(x); xx < Math.round(x) + Math.round(w); xx++) px(xx, yy, c);
  }

  function recta(x, y, w, h, c, a) {
    for (let yy = Math.round(y); yy < Math.round(y) + Math.round(h); yy++)
      for (let xx = Math.round(x); xx < Math.round(x) + Math.round(w); xx++) pxa(xx, yy, c, a);
  }

  function circle(cx, cy, r, c) { ell(cx, cy, r, r, c); }

  function ell(cx, cy, rx, ry, c) {
    if (rx < 0.5) rx = 0.5;
    if (ry < 0.5) ry = 0.5;
    for (let y = Math.ceil(cy - ry); y <= Math.floor(cy + ry); y++) {
      const dy = (y - cy) / ry;
      const k = 1 - dy * dy;
      if (k < 0) continue;
      const dx = rx * Math.sqrt(k);
      for (let x = Math.ceil(cx - dx); x <= Math.floor(cx + dx); x++) px(x, y, c);
    }
  }

  // Filled triangle by edge function, sampling pixel centers. Matches the
  // C++ port exactly because both use the same +0.5 sample point.
  function tri(x0, y0, x1, y1, x2, y2, c) {
    const minx = Math.floor(Math.min(x0, x1, x2)), maxx = Math.ceil(Math.max(x0, x1, x2));
    const miny = Math.floor(Math.min(y0, y1, y2)), maxy = Math.ceil(Math.max(y0, y1, y2));
    for (let y = miny; y <= maxy; y++) {
      for (let x = minx; x <= maxx; x++) {
        const sx = x + 0.5, sy = y + 0.5;
        const e0 = (sx - x0) * (y1 - y0) - (sy - y0) * (x1 - x0);
        const e1 = (sx - x1) * (y2 - y1) - (sy - y1) * (x2 - x1);
        const e2 = (sx - x2) * (y0 - y2) - (sy - y2) * (x0 - x2);
        if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0)) px(x, y, c);
      }
    }
  }

  function quad(x0, y0, x1, y1, x2, y2, x3, y3, c) {
    tri(x0, y0, x1, y1, x2, y2, c);
    tri(x0, y0, x2, y2, x3, y3, c);
  }

  // Thick line by stamping a disc along a Bresenham walk, with an optional
  // taper from thick0 at the start to thick1 at the end.
  function line(x0, y0, x1, y1, c, thick0, thick1) {
    if (thick0 === undefined) thick0 = 1;
    if (thick1 === undefined) thick1 = thick0;
    const steps = Math.max(1, Math.ceil(Math.hypot(x1 - x0, y1 - y0) * 2));
    for (let i = 0; i <= steps; i++) {
      const f = i / steps;
      const x = x0 + (x1 - x0) * f, y = y0 + (y1 - y0) * f;
      const t = thick0 + (thick1 - thick0) * f;
      if (t <= 1.2) px(x, y, c); else circle(x, y, t / 2, c);
    }
  }

  // Deterministic hash noise - identical in JS and C++ so flicker patterns
  // match between the simulator and the panel.
  function hash(n) {
    n = (n | 0) & 0x7fffffff;
    n = (n ^ 61) ^ (n >>> 16);
    n = (n + (n << 3)) & 0x7fffffff;
    n = n ^ (n >>> 4);
    n = Math.imul(n, 0x27d4eb2d) & 0x7fffffff;
    n = n ^ (n >>> 15);
    return (n & 0x7fffffff) / 0x7fffffff;
  }
  function hash2(a, b) { return hash((a | 0) * 73856093 ^ (b | 0) * 19349663); }

  function lerp(a, b, f) { return a + (b - a) * f; }
  function mix(c0, c1, f) {
    if (f < 0) f = 0; if (f > 1) f = 1;
    return C(lerp(cr(c0), cr(c1), f), lerp(cg(c0), cg(c1), f), lerp(cb(c0), cb(c1), f));
  }
  function scaleC(c, f) { return C(cr(c) * f, cg(c) * f, cb(c) * f); }
  function clamp(v, lo, hi) { return v < lo ? lo : v > hi ? hi : v; }
  function smooth(f) { f = clamp(f, 0, 1); return f * f * (3 - 2 * f); }
  // 0 -> 1 -> 0 over the window [a,b]
  function pulse(t, a, b) {
    if (t < a || t > b) return 0;
    const f = (t - a) / (b - a);
    return Math.sin(f * Math.PI);
  }

  // ---- Text: the Adafruit_GFX built-in 5x7 glyphs, so what the simulator
  // measures is what the panel draws. FONT is injected by font.js. ----
  let FONT = null;
  function setFont(f) { FONT = f; }

  function textWidth(s, scale) { return s.length * 6 * scale - scale; }

  function drawChar(ch, x, y, scale, c) {
    const cols = FONT[ch] || FONT['?'];
    if (!cols) return;
    for (let col = 0; col < 5; col++) {
      const bits = cols[col];
      for (let row = 0; row < 7; row++) {
        if (!((bits >> row) & 1)) continue;
        if (scale === 1) px(x + col, y + row, c);
        else rect(x + col * scale, y + row * scale, scale, scale, c);
      }
    }
  }

  function text(s, x, y, scale, c) {
    s = String(s).toUpperCase();
    for (let i = 0; i < s.length; i++) drawChar(s[i], x + i * 6 * scale, y, scale, c);
  }

  function textCentered(s, y, scale, c) {
    text(s, Math.round((W - textWidth(String(s), scale)) / 2), y, scale, c);
  }

  global.BS = {
    W, H, buf, C, cr, cg, cb, px, pxa, getPx, clear, rect, recta, circle, ell,
    tri, quad, line, hash, hash2, lerp, mix, scaleC, clamp, smooth, pulse,
    setClip, resetClip, setFont, text, textCentered, textWidth, drawChar,
  };
})(typeof module !== 'undefined' ? module.exports : (typeof window !== 'undefined' ? window : globalThis));
if (typeof module !== 'undefined') module.exports = module.exports.BS;
