/*
 * NEON PULSE LAB - the everyday face of the sign.
 *
 * The logo is fixed in the top half, a lamp sits in each corner (red for
 * busy, green for come in), and the bottom half belongs to the jet. With
 * no message it flies in, barrel-rolls at centre and carries on out to
 * the left; with a message it flies straight through with the text
 * trailing in its wake. Nothing ever cuts to a different screen.
 *
 * Like the scenes, the face is a pure function of elapsed seconds, so the
 * panel and the simulator draw the same frame from the same clock.
 */
(function (global) {
  const B = global.BS || require('./engine.js');
  const {
    W, C, px, pxa, ell, tri, quad, line, hash2, mix, scaleC, clamp, smooth,
    setClip, resetClip, text, textWidth,
  } = B;

  // ---- Brand palette (npts-branding: lockup, wallpaper, service label) ----
  const CYAN      = C(0, 229, 255);
  const MAGENTA   = C(255, 45, 155);
  const MAG_ECHO  = C(96, 10, 60);
  const LAMP_BUSY = C(255, 22, 22);
  const LAMP_OPEN = C(20, 235, 80);
  const BLACK     = C(0, 0, 0);

  const JET_BODY  = C(58, 74, 104);
  const JET_RIM   = C(120, 220, 255);
  const WING_TOP  = C(84, 104, 140);
  const WING_BELLY = C(30, 38, 58);
  const FIN       = C(70, 90, 126);
  const CANOPY    = C(170, 232, 255);
  const FLAME_HOT = C(255, 236, 170);
  const FLAME     = C(255, 120, 40);
  const WAKE_FAR  = C(120, 30, 90);
  const TEXT_HOT  = C(255, 240, 200);
  const TEXT      = C(255, 150, 58);

  // The 16-band sunset ramp the wallpaper, badges and labels are built on.
  const STOPS = [
    [0.00, [10, 22, 48]], [0.28, [13, 58, 86]], [0.52, [92, 34, 64]],
    [0.74, [196, 64, 32]], [1.00, [255, 150, 58]],
  ];
  function ramp(f) {
    for (let i = 0; i < STOPS.length - 1; i++) {
      const a = STOPS[i], b = STOPS[i + 1];
      if (f >= a[0] && f <= b[0]) {
        const k = (f - a[0]) / (b[0] - a[0]);
        return C(a[1][0] + (b[1][0] - a[1][0]) * k,
                 a[1][1] + (b[1][1] - a[1][1]) * k,
                 a[1][2] + (b[1][2] - a[1][2]) * k);
      }
    }
    const l = STOPS[STOPS.length - 1][1];
    return C(l[0], l[1], l[2]);
  }
  // The ramp's navy end is nearly black on an LED, so the rules take each
  // band's hue at a common brightness instead of its printed value.
  function band(x) {
    const c = ramp((Math.floor(x / 4) + 0.5) / 16);
    const m = Math.max(B.cr(c), B.cg(c), B.cb(c));
    return scaleC(c, 210 / m);
  }

  // ---- Logo lettering ----
  // Hand-cut 7-row glyphs after the NEON PULSE lockup: square shoulders,
  // notched corners and the stepped N. Only the letters the sign uses.
  const GLYPHS = {
    N: ['#...#', '##..#', '##..#', '#.#.#', '#..##', '#..##', '#...#'],
    E: ['####', '#...', '#...', '###.', '#...', '#...', '####'],
    O: ['.###.', '#...#', '#...#', '#...#', '#...#', '#...#', '.###.'],
    P: ['###.', '#..#', '#..#', '###.', '#...', '#...', '#...'],
    U: ['#...#', '#...#', '#...#', '#...#', '#...#', '#...#', '.###.'],
    L: ['#...', '#...', '#...', '#...', '#...', '#...', '####'],
    S: ['####', '#...', '#...', '####', '...#', '...#', '####'],
    A: ['.###.', '#...#', '#...#', '#####', '#...#', '#...#', '#...#'],
    B: ['###.', '#..#', '#..#', '###.', '#..#', '#..#', '###.'],
  };

  function glyph(ch, x, y, c) {
    const g = GLYPHS[ch];
    for (let r = 0; r < 7; r++)
      for (let k = 0; k < g[r].length; k++)
        if (g[r][k] === '#') px(x + k, y + r, c);
  }

  function wordWidth(s, gap, space) {
    let w = 0;
    for (let i = 0; i < s.length; i++) {
      w += s[i] === ' ' ? space : GLYPHS[s[i]][0].length;
      if (i < s.length - 1) w += gap;
    }
    return w;
  }

  // The lockup's echo: the same letters again one pixel down-right in the
  // partner colour, drawn first so the face sits on top of it.
  function word(s, x, y, gap, space, face, echo) {
    for (let pass = 0; pass < 2; pass++) {
      let cx = x;
      for (let i = 0; i < s.length; i++) {
        if (s[i] === ' ') { cx += space + gap; continue; }
        if (pass === 1) glyph(s[i], cx, y, face);
        else if (echo !== null) glyph(s[i], cx + 1, y + 1, echo);
        cx += GLYPHS[s[i]][0].length + gap;
      }
    }
  }

  function logo() {
    const top = 'NEON PULSE';
    word(top, Math.round((W - wordWidth(top, 1, 2)) / 2), 1, 1, 2, CYAN, MAG_ECHO);
    // LAB goes without the echo: a cyan echo under magenta turned the
    // counters to mud, and the row it needs belongs to the jet.
    const lw = wordWidth('LAB', 2, 0);
    const lx = Math.round((W - lw) / 2);
    word('LAB', lx, 9, 2, 0, MAGENTA, null);
    // Ramp-coloured rules either side of LAB, doubled like the lockup's
    // outline: a lit tube with a dimmer echo under it.
    for (let x = 7; x < lx - 3; x++) { px(x, 11, band(x)); px(x, 13, scaleC(band(x), 0.45)); }
    for (let x = lx + lw + 4; x <= 56; x++) { px(x, 11, band(x)); px(x, 13, scaleC(band(x), 0.45)); }
  }

  // ---- Corner lamps ----
  const LAMPS = [[2, 2], [61, 2], [2, 29], [61, 29]];
  const LAMP_BREATH = 3.0;

  function lamps(t, busy) {
    const base = busy ? LAMP_BUSY : LAMP_OPEN;
    // A slow breath so they read as lit lamps rather than painted dots. Three
    // seconds, so the idle pass is a whole number of breaths and the panel
    // can wrap its clock at a pass boundary without a visible step.
    const b = 0.80 + 0.20 * Math.sin((t * 2 * Math.PI) / LAMP_BREATH);
    for (let i = 0; i < 4; i++) {
      const x = LAMPS[i][0], y = LAMPS[i][1];
      ell(x, y, 3.4, 3.4, BLACK);
      ell(x, y, 2.4, 2.4, scaleC(base, 0.45 * b));
      ell(x, y, 1.5, 1.5, scaleC(base, b));
      px(x - 1, y - 1, mix(scaleC(base, b), C(255, 255, 255), 0.45));
    }
  }

  // ---- The jet ----
  // Model space: x runs nose (0) to tail (JET_LEN), y is up, z is toward
  // the viewer. The fuselage is treated as round, so its side profile never
  // changes; canopy, wings, fin and tailplanes are 3D and turn with a roll.
  const JET_LEN = 20;
  const FUSE = [
    [0, 0.3], [2.5, -0.9], [6, -1.4], [15, -1.4], [19.6, -1.0],
    [19.6, 1.0], [14, 1.4], [4, 1.3], [1.5, 0.9],
  ];
  const PARTS = [
    { kind: 'wing', pts: [[7, -0.3, 0.6], [15, -0.3, 5.4], [16.8, -0.3, 5.4], [17.6, -0.3, 0.6]] },
    { kind: 'wing', pts: [[7, -0.3, -0.6], [15, -0.3, -5.4], [16.8, -0.3, -5.4], [17.6, -0.3, -0.6]] },
    { kind: 'wing', pts: [[16, 0, 0.6], [18.8, 0, 2.6], [19.8, 0, 2.6], [19.8, 0, 0.6]] },
    { kind: 'wing', pts: [[16, 0, -0.6], [18.8, 0, -2.6], [19.8, 0, -2.6], [19.8, 0, -0.6]] },
    { kind: 'fin',  pts: [[13.6, 1.0, 0], [17.4, 4.8, 0], [19.4, 4.8, 0], [19.6, 1.0, 0]] },
  ];
  // Resting roll: banked a little toward the viewer so the near wing shows
  // below the fuselage, the way the wallpaper jet is drawn.
  const ROLL0 = 0.42;

  function jet(jx, jy, roll) {
    const cs = Math.cos(roll), sn = Math.sin(roll);
    // Project every part, then paint far-to-near around the fuselage.
    const items = [];
    for (let i = 0; i < PARTS.length; i++) {
      const p = PARTS[i];
      const scr = [];
      let zsum = 0;
      for (let k = 0; k < 4; k++) {
        const [x, y, z] = p.pts[k];
        const y2 = y * cs - z * sn, z2 = y * sn + z * cs;
        scr.push([jx + x, jy - y2]);
        zsum += z2;
      }
      // Wings face +y, the fin faces +z. Which side we see sets the colour,
      // how squarely we see it sets the brightness.
      let col;
      if (p.kind === 'wing') col = scaleC(sn >= 0 ? WING_TOP : WING_BELLY, 0.55 + 0.45 * Math.abs(sn));
      else col = scaleC(FIN, 0.6 + 0.4 * Math.abs(cs));
      items.push({ z: zsum / 4, scr, col, fin: p.kind === 'fin' });
    }
    const canY = 1.5 * cs, canZ = 1.5 * sn;
    items.push({ z: canZ, canopy: true });
    items.sort((a, b) => a.z - b.z);

    function draw(it) {
      if (it.canopy) {
        ell(jx + 6.2, jy - canY, 2.2, 0.8, CANOPY);
        return;
      }
      const s = it.scr;
      quad(s[0][0], s[0][1], s[1][0], s[1][1], s[2][0], s[2][1], s[3][0], s[3][1], it.col);
      // The wallpaper jet's fin has a lit leading edge; it fades out as the
      // fin turns edge-on.
      if (it.fin && Math.abs(cs) > 0.35) line(s[0][0], s[0][1], s[1][0], s[1][1], JET_RIM);
    }

    let i = 0;
    for (; i < items.length && items[i].z < 0; i++) draw(items[i]);
    for (let k = 1; k < FUSE.length - 1; k++)
      tri(jx + FUSE[0][0], jy + FUSE[0][1], jx + FUSE[k][0], jy + FUSE[k][1],
          jx + FUSE[k + 1][0], jy + FUSE[k + 1][1], JET_BODY);
    // Light comes from above, so the rim is the top edge of the profile
    // whichever way up the jet is.
    for (let x = 3; x <= 18; x++) px(jx + x, jy - 1.4, JET_RIM);
    px(jx + 1.6, jy - 0.6, JET_RIM);
    px(jx + 0.4, jy + 0.1, JET_RIM);
    for (; i < items.length; i++) draw(items[i]);
  }

  function flame(nx, ny, t) {
    const f = hash2(Math.floor(t * 24), 7);
    const len = 1.6 + f * 1.8;
    ell(nx + len * 0.5, ny, len * 0.6 + 0.4, 0.9, FLAME);
    px(nx + 0.5, ny, FLAME_HOT);
  }

  // ---- Flight paths ----
  // Idle pass: cruise in from the right, ease off and roll at centre,
  // cruise out to the left, then a quiet gap with just the logo.
  const AXIS_Y = 23;
  const VC = 26;              // cruise, px/s
  const VR = 7;               // slowest point of the roll
  const SLOW0 = 1.15, SLOW_D = 2.4;
  const ROLL_A = 1.55, ROLL_D = 1.6;
  const HELIX_R = 2.0;
  const IDLE_CYCLE = 9.0;

  // Distance lost to the slowdown: integral of (VC-VR)*sin^2 over the window.
  function slowLoss(t) {
    const u = clamp(t - SLOW0, 0, SLOW_D);
    return (VC - VR) * (u / 2 - (SLOW_D / (4 * Math.PI)) * Math.sin((2 * Math.PI * u) / SLOW_D));
  }

  function idlePose(t) {
    const x = W - VC * t + slowLoss(t);
    const r = smooth((t - ROLL_A) / ROLL_D);
    const phi = r * 2 * Math.PI;
    // Climb over the top of the roll, inverted at the apex: a barrel roll
    // seen side-on rather than a flat aileron roll.
    const y = AXIS_Y - HELIX_R * (1 - Math.cos(phi));
    return [x, y, ROLL0 + phi];
  }

  function idle(t) {
    const tt = t % IDLE_CYCLE;
    const [x, y, roll] = idlePose(tt);
    if (x > W + 4 || x < -JET_LEN - 30) return;
    // Exhaust trail: where the nozzle has been, so the corkscrew of the
    // roll stays drawn in the air behind it.
    for (let k = 26; k >= 1; k--) {
      const tp = tt - k * 0.03;
      if (tp < 0) continue;
      const [px0, py0] = idlePose(tp);
      const f = k / 26;
      pxa(px0 + JET_LEN - 0.4, py0, mix(FLAME, WAKE_FAR, f), (1 - f) * 0.85);
    }
    flame(x + JET_LEN - 0.4, y, t);
    jet(x, y, roll);
  }

  // Message pass: straight and level at a reading pace, the text riding in
  // the exhaust. Letters are hottest right behind the nozzle and cool to
  // the settled colour a couple of characters back.
  const VM = 20;
  const MSG_GAP = 5;
  const MSG_PAUSE = 1.0;

  function msgCycle(msg) {
    return (W + JET_LEN + MSG_GAP + textWidth(msg, 1) + 2) / VM + MSG_PAUSE;
  }

  function message(t, msg) {
    const tt = t % msgCycle(msg);
    const x = W - VM * tt;
    const y = AXIS_Y;
    const nx = x + JET_LEN - 0.4;
    const tx = nx + MSG_GAP;
    for (let k = 0; k < 4; k++) {
      const sx = nx + 1 + k;
      pxa(sx, y, mix(FLAME, WAKE_FAR, k / 4), 0.7 - k * 0.15);
    }
    const s = String(msg).toUpperCase();
    for (let i = 0; i < s.length; i++) {
      const cx = tx + i * 6;
      if (cx > W || cx < -6) continue;
      const heat = clamp(1 - (cx - tx) / 18, 0, 1);
      text(s[i], cx, y - 3, 1, mix(TEXT, TEXT_HOT, heat * heat));
    }
    flame(nx, y, t);
    jet(x, y, ROLL0);
  }

  function frame(t, busy, msg) {
    logo();
    setClip(0, 16, W - 1, B.H - 1);
    if (msg) message(t, msg);
    else idle(t);
    resetClip();
    lamps(t, busy);
  }

  const api = { frame, msgCycle, IDLE_CYCLE, GLYPHS };
  global.LAB = api;
  if (typeof module !== 'undefined') module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
