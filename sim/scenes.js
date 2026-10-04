/*
 * Jurassic Park birthday show - scene definitions.
 *
 * Each scene is a pure function of elapsed seconds, so the simulator can
 * scrub anywhere in the timeline and the firmware can render the same
 * frame from millis() with no per-scene state to keep in sync.
 */
(function (global) {
  const B = global.BS || require('./engine.js');
  const {
    W, H, C, px, pxa, clear, rect, recta, circle, ell, tri, quad, line,
    hash, hash2, mix, scaleC, clamp, smooth, pulse, setClip, resetClip,
    text, textCentered, textWidth,
  } = B;

  // ---- Palette ----
  const AMBER     = C(255, 168, 40);
  const AMBER_HOT = C(255, 226, 140);
  const AMBER_DIM = C(150, 92, 18);
  const JP_RED    = C(198, 38, 30);
  const JP_ORANGE = C(233, 112, 26);
  const NIGHT     = C(4, 6, 12);
  const JUNGLE    = C(7, 22, 13);
  const JUNGLE_LT = C(15, 44, 24);
  const WOOD      = C(104, 62, 28);
  const WOOD_DK   = C(58, 33, 14);
  const WOOD_LT   = C(140, 88, 42);
  const STONE     = C(78, 68, 58);
  const STONE_DK  = C(42, 36, 30);
  const STONE_LT  = C(112, 100, 88);
  const BONE      = C(240, 230, 206);
  const BLACK     = C(0, 0, 0);
  const SIL       = C(6, 5, 8);      // foreground silhouette
  const SIL_FAR   = C(26, 18, 16);   // far limbs, for depth
  const WHITE     = C(244, 244, 240);

  function rot(x, y, cx, cy, a) {
    const s = Math.sin(a), c = Math.cos(a);
    const dx = x - cx, dy = y - cy;
    return [cx + dx * c - dy * s, cy + dx * s + dy * c];
  }

  // =====================================================================
  // Shared set pieces
  // =====================================================================

  // Warm sunset sky: deep red at the top falling to hot amber at the horizon.
  function sunsetSky(horizon) {
    for (let y = 0; y < horizon; y++) {
      const f = y / Math.max(1, horizon - 1);
      const c = mix(C(52, 8, 26), C(246, 138, 28), f * f);
      for (let x = 0; x < W; x++) px(x, y, c);
    }
  }

  function nightSky(t) {
    clear(NIGHT);
    for (let i = 0; i < 26; i++) {
      const x = Math.floor(hash(i * 7 + 1) * W);
      const y = Math.floor(hash(i * 13 + 5) * 18);
      const tw = 0.35 + 0.65 * Math.abs(Math.sin(t * 1.4 + i));
      pxa(x, y, C(200, 220, 255), 0.25 + tw * 0.5);
    }
  }

  // Ragged canopy silhouette with a couple of fern fronds poking up.
  function treeline(topY, col, seed) {
    for (let x = 0; x < W; x++) {
      const n = hash2(x + seed, seed) * 3 + hash2(Math.floor(x / 5) + seed, seed * 3) * 3;
      const top = topY + Math.floor(n) - 2;
      for (let y = top; y < H; y++) px(x, y, col);
    }
    for (let i = 0; i < 5; i++) {
      const fx = 4 + Math.floor(hash(i * 31 + seed) * (W - 8));
      const fh = 5 + Math.floor(hash(i * 17 + seed) * 5);
      line(fx, topY + 2, fx - 2, topY + 2 - fh, col, 1.6, 1);
      for (let k = 1; k <= 3; k++) {
        const fy = topY + 2 - (fh * k) / 4;
        line(fx - k * 0.5, fy, fx - k * 0.5 - 3, fy - 1.5, col, 1.2, 1);
        line(fx - k * 0.5, fy, fx - k * 0.5 + 3, fy - 1.5, col, 1.2, 1);
      }
    }
  }

  // Flickering torch flame rising from (cx, baseY).
  function torch(cx, baseY, t, seed) {
    const f = hash2(Math.floor(t * 14) + seed, seed);
    const f2 = hash2(Math.floor(t * 9) + seed * 3, seed);
    const hgt = 5.0 + f * 2.2;
    for (let i = 0; i <= 10; i++) {
      const k = i / 10;
      const y = baseY - k * hgt;
      const sway = Math.sin(t * 6 + seed + k * 3) * (0.8 + k * 1.4) * (0.6 + f2 * 0.8);
      const wdt = (1 - k) * 2.3 + 0.5;
      const col = mix(mix(C(255, 60, 0), AMBER, k * 0.8), AMBER_HOT, k * k);
      circle(cx + sway * 0.5, y, wdt, col);
    }
    // Pool of light on whatever is under the torch.
    for (let y = baseY - 4; y < baseY + 9; y++)
      for (let x = cx - 6; x <= cx + 6; x++) {
        const d = Math.hypot(x - cx, (y - baseY) * 0.8);
        if (d < 6.5) pxa(x, y, AMBER, (1 - d / 6.5) * 0.22 * (0.75 + f * 0.5));
      }
  }

  // ---- Tyrannosaurus, in profile facing left ----
  // Feet land on y=27 and the belly stops at y=19, so eight rows of sky
  // show under it and between the legs. That negative space is the whole
  // reason the shape reads as a rex instead of a black smear.
  // jaw: radians the lower jaw drops. lift: radians the whole head tips up.
  function trexBody(ox, oy, col, farCol, jaw, lift, stride) {
    // Tail.
    line(ox + 40, oy + 13, ox + 57, oy + 7.5, col, 6, 1.2);

    // Far leg behind the body.
    const fs = Math.sin(stride) * 1.6;
    ell(ox + 39.5, oy + 17, 3.2, 4.0, farCol);
    line(ox + 39.5, oy + 20, ox + 41.5 + fs, oy + 25.6, farCol, 2.2, 1.8);
    rect(ox + 40 + fs, oy + 25.6, 5.5, 1.5, farCol);

    // Barrel body.
    ell(ox + 34, oy + 14, 8, 4.6, col);

    // Near leg in front: long enough to read as a leg.
    const ns = Math.sin(stride + Math.PI) * 1.6;
    ell(ox + 35.5, oy + 17.5, 3.6, 4.4, col);
    line(ox + 35.5, oy + 21, ox + 33 + ns, oy + 25.8, col, 2.6, 2);
    rect(ox + 30.5 + ns, oy + 25.6, 6, 1.7, col);

    // Vestigial arm.
    line(ox + 29.5, oy + 14, ox + 27.4, oy + 16.6, col, 2.2, 1.6);
    px(ox + 26.6, oy + 17.2, col);

    // Neck - deliberately thinner than head and body so a notch shows.
    line(ox + 27, oy + 12, ox + 21.5, oy + 9.4, col, 5.2, 4.4);

    // Head rotates about the jaw hinge so skull and jaw lift together.
    const hx = ox + 20.5, hy = oy + 10;
    const R = (x, y) => rot(x + ox, y + oy, hx, hy, -lift);
    const RJ = (x, y) => {
      const [jx, jy] = rot(x + ox, y + oy, ox + 20, oy + 10.6, jaw);
      return rot(jx, jy, hx, hy, -lift);
    };

    // Upper jaw: brow slope down to the snout, flat biting edge under it.
    const a1 = R(9.5, 8.8), a2 = R(11.5, 6.2), a3 = R(17.5, 5.4), a4 = R(21, 8.8);
    const a5 = R(21, 10.1), a6 = R(9.8, 10.1);
    quad(a1[0], a1[1], a2[0], a2[1], a3[0], a3[1], a4[0], a4[1], col);
    quad(a1[0], a1[1], a4[0], a4[1], a5[0], a5[1], a6[0], a6[1], col);

    // Lower jaw.
    const b1 = RJ(10.4, 10.6), b2 = RJ(19.4, 10.3), b3 = RJ(20, 12.2), b4 = RJ(11.2, 12.5);
    quad(b1[0], b1[1], b2[0], b2[1], b3[0], b3[1], b4[0], b4[1], col);

    // Teeth only once there is a gap to show them in.
    if (jaw > 0.10) {
      for (let i = 0; i < 4; i++) {
        const fx = 11.4 + i * 2.2;
        const u = R(fx, 10.1);
        tri(u[0], u[1], u[0] + 1.1, u[1], u[0] + 0.5, u[1] + 1.4, BONE);
        const l = RJ(fx + 0.7, 10.7);
        tri(l[0], l[1], l[0] + 1.1, l[1], l[0] + 0.5, l[1] - 1.4, BONE);
      }
    }

    // Eye.
    const e = R(16.6, 7.4);
    px(e[0], e[1], AMBER_HOT);
    return R(10, 10.6); // mouth anchor, for the roar rings
  }

  // Draws the rex with a one-pixel amber rim on its upper edges: the same
  // silhouette stamped once in dim amber a pixel higher, then black on top.
  function trex(ox, oy, jaw, lift, stride) {
    trexBody(ox, oy - 1, AMBER_DIM, scaleC(AMBER_DIM, 0.55), jaw, lift, stride);
    return trexBody(ox, oy, SIL, SIL_FAR, jaw, lift, stride);
  }

  // Compact rex silhouette, 27x18 with the feet on oy+17. A solid shape
  // reads far better inside the emblem than a skeleton would at this size.
  function miniRex(ox, oy, col, far) {
    line(ox + 19, oy + 7.5, ox + 26, oy + 4, col, 4, 1);
    ell(ox + 18.5, oy + 11, 2, 2.6, far);
    line(ox + 18.5, oy + 12.5, ox + 20, oy + 15.5, far, 1.6);
    rect(ox + 19.5, oy + 15.6, 3.5, 1.2, far);
    ell(ox + 15, oy + 8.5, 5.5, 3.2, col);
    ell(ox + 16, oy + 11, 2.4, 3, col);
    line(ox + 16, oy + 13, ox + 14.5, oy + 16, col, 1.8);
    rect(ox + 12.5, oy + 15.8, 4.5, 1.3, col);
    line(ox + 12, oy + 7, ox + 9.5, oy + 5.5, col, 3, 2.6);
    quad(ox + 1, oy + 4.4, ox + 2.5, oy + 2.8, ox + 7, oy + 2.4, ox + 9.5, oy + 4.4, col);
    quad(ox + 1, oy + 4.4, ox + 9.5, oy + 4.4, ox + 9.5, oy + 5.4, ox + 1.2, oy + 5.4, col);
    quad(ox + 1.6, oy + 5.9, ox + 8.6, oy + 5.7, ox + 9, oy + 7, ox + 2.2, oy + 7.2, col);
    line(ox + 12.5, oy + 8.5, ox + 10.8, oy + 10.3, col, 1.6);
  }

  // Three-toed print pressed into the dirt. f ramps 0..1 as it lands.
  function footprint(cx, cy, f) {
    if (f <= 0) return;
    const dirt = C(44, 26, 12);
    const r = 0.6 + f * 1.0;
    ell(cx, cy, 3.2 * f, 2.0 * f, dirt);
    for (let i = -1; i <= 1; i++)
      tri(cx + i * 2.6, cy - 1.5, cx + i * 2.6 - 1.1, cy - 1.5,
          cx + i * 3.1, cy - 2.4 - 2.2 * f, dirt);
    // Dust ring on impact.
    if (f < 1) for (let a = 0; a < 6.2; a += 0.5)
      pxa(cx + Math.cos(a) * (4 + f * 5), cy + Math.sin(a) * (1.6 + f * 2), C(150, 120, 84), (1 - f) * 0.6);
    void r;
  }

  // ---- Velociraptor head in profile facing right ----
  function raptorHead(ox, oy, t, blink, jaw, bodyCol, stripeCol) {
    const dark = scaleC(bodyCol, 0.55);
    // Neck sweeping off the bottom-left corner.
    quad(ox + 0, oy + 16, ox + 18, oy + 1, ox + 24, oy + 9, ox + 6, oy + 20, bodyCol);
    rect(ox + 0, oy + 12, 8, 10, bodyCol);
    // Skull.
    ell(ox + 26, oy + 6, 10, 7.5, bodyCol);
    // Snout tapering forward.
    quad(ox + 32, oy + 1.8, ox + 46, oy + 5.6, ox + 46, oy + 8.6, ox + 32, oy + 11.5, bodyCol);
    // Lower jaw, hinged at the back so a snap opens the front.
    const JH = [ox + 32, oy + 11];
    const J = (x, y) => rot(x, y, JH[0], JH[1], jaw);
    let j1 = J(ox + 32, oy + 10), j2 = J(ox + 45, oy + 9.3),
        j3 = J(ox + 45.5, oy + 11.6), j4 = J(ox + 32, oy + 13.5);
    quad(j1[0], j1[1], j2[0], j2[1], j3[0], j3[1], j4[0], j4[1], bodyCol);
    // Teeth along both jaw lines.
    for (let i = 0; i < 6; i++) {
      const fx = ox + 34 + i * 1.9;
      px(fx, oy + 9.4 + i * 0.16, BONE);
      const lt = J(fx + 0.6, oy + 9.6);
      px(lt[0], lt[1], BONE);
    }
    // Mouth shadow so the jaws separate visually.
    line(ox + 32, oy + 10.6, ox + 45, oy + 8.8, dark, 1);
    // Brow ridge and eye.
    quad(ox + 20, oy + 1.5, ox + 33, oy + 1.2, ox + 33, oy + 3.4, ox + 20, oy + 4.2, dark);
    if (blink < 0.5) {
      circle(ox + 27, oy + 5.4, 2.2, C(255, 205, 45));
      rect(ox + 26.6, oy + 3.8, 1, 3.4, C(20, 12, 6));
      px(ox + 28, oy + 4.4, C(255, 250, 220));
    } else {
      circle(ox + 27, oy + 5.4, 2.2, scaleC(bodyCol, 0.8));
      line(ox + 25, oy + 5.6, ox + 29, oy + 5.4, dark, 1);
    }
    // Nostril.
    px(ox + 43.5, oy + 6.4, dark);
    // Signature stripe: snout, over the skull, down the neck.
    const pts = [[45, 4.8], [38, 2.8], [30, 0.9], [22, 0.5], [13, 5.2], [5, 12], [1, 19]];
    const core = mix(stripeCol, WHITE, 0.42);
    for (let i = 0; i < pts.length - 1; i++) {
      const A = pts[i], Bp = pts[i + 1];
      line(ox + A[0], oy + A[1], ox + Bp[0], oy + Bp[1], stripeCol, 2.6, 2.6);
    }
    for (let i = 0; i < pts.length - 1; i++) {
      const A = pts[i], Bp = pts[i + 1];
      line(ox + A[0], oy + A[1] + 0.3, ox + Bp[0], oy + Bp[1] + 0.3, core, 1, 1);
    }
    // Scale speckle.
    for (let i = 0; i < 30; i++) {
      const sx = ox + 4 + hash(i * 11 + 3) * 44;
      const sy = oy + 2 + hash(i * 19 + 7) * 18;
      pxa(sx, sy, dark, 0.5);
    }
  }

  // ---- JP tour Jeep, driving right ----
  // 32 long by 23 tall including the roll bar, sized so the door number
  // fits the body at scale 1. y is the top of the body tub.
  function jeep(x, y, t) {
    const CREAM = C(228, 212, 170);
    const CREAM_DK = C(160, 144, 110);
    const STRIPE = C(208, 60, 32);
    const CAGE = C(48, 50, 54);
    const TIRE = C(16, 16, 20);

    // Dust boiling up behind the rear wheel.
    for (let i = 0; i < 6; i++) {
      const age = (t * 2.2 + i * 0.31) % 1;
      const dx = x + 3 - age * 20, dy = y + 12 - age * 4 - hash(i * 23) * 2;
      const r = 1.4 + age * 4;
      for (let yy = Math.floor(dy - r); yy <= dy + r; yy++)
        for (let xx = Math.floor(dx - r); xx <= dx + r; xx++)
          if (Math.hypot(xx - dx, yy - dy) <= r) pxa(xx, yy, C(152, 124, 90), (1 - age) * 0.40);
    }

    // Roll bar and windshield frame.
    rect(x + 7, y - 5, 13, 1, CAGE);
    rect(x + 7, y - 5, 1, 5, CAGE);
    line(x + 19.5, y, x + 20.5, y - 5, CAGE, 1.4);

    // Windscreen glass behind the frame.
    quad(x + 16, y - 4, x + 19.6, y - 4, x + 20, y - 0.5, x + 16, y - 0.5, C(118, 142, 150));
    // Body tub, then a clearly lower hood ahead of the cowl.
    rect(x + 1, y, 20, 9, CREAM);
    rect(x + 20, y + 4, 10, 5, CREAM);
    rect(x + 1, y + 8, 29, 1, CREAM_DK);
    rect(x + 20, y + 3, 10, 1, CREAM_DK);
    // Spare wheel on the tailgate.
    circle(x + 0.5, y + 4, 2.6, C(28, 28, 32));
    circle(x + 0.5, y + 4, 1.1, CREAM_DK);

    // Livery: the band under the doors plus the fender flash.
    rect(x + 2, y + 7, 28, 1, STRIPE);
    rect(x + 21, y + 5, 8, 1, STRIPE);
    rect(x + 19, y, 1, 8, STRIPE);

    // Grille and headlight.
    rect(x + 30, y + 4, 1, 5, C(60, 56, 50));
    px(x + 30, y + 5, AMBER_HOT);
    px(x + 30, y + 7, AMBER_HOT);
    for (let i = 1; i < 10; i++) pxa(x + 31 + i, y + 6, AMBER, 0.32 - i * 0.031);

    // Door number.
    text('10', x + 9, y + 1, 1, C(30, 28, 26));

    // Wheels over the body so the arches read.
    for (const wx of [x + 7, x + 24]) {
      circle(wx, y + 12, 4, TIRE);
      circle(wx, y + 12, 1.8, C(162, 162, 166));
      const a = t * 8 + (wx === x + 7 ? 0 : 1.2);
      px(wx + Math.cos(a) * 2.8, y + 12 + Math.sin(a) * 2.8, C(92, 92, 96));
    }
  }

  // =====================================================================
  // Scenes
  // =====================================================================

  // ---- 1. The gates ----
  function sceneGate(t) {
    const openF = smooth((t - 2.2) / 2.6);
    const gap = openF * 21;

    nightSky(t);
    // Amber floodlight spilling out from inside the park.
    for (let y = 10; y < H; y++)
      for (let x = 10; x < 54; x++) {
        const d = Math.hypot((x - 32) * 0.7, y - 30);
        pxa(x, y, JP_ORANGE, clamp((1 - d / 26) * 0.85 * openF, 0, 1));
      }
    // What's waiting inside: canopy, amber light spilling through it, and
    // a pair of eyes. Eyes read at this size; a small body never will.
    setClip(10, 12, 53, 31);
    for (let x = 10; x < 54; x++) {
      const top = 19 + Math.floor(hash2(x, 5) * 3 + hash2(Math.floor(x / 6), 13) * 4);
      for (let y = top; y < 32; y++) px(x, y, C(7, 20, 12));
    }
    for (let y = 12; y < 32; y++)
      for (let x = 10; x < 54; x++) {
        const d = Math.hypot((x - 32) * 0.7, y - 31);
        pxa(x, y, JP_ORANGE, clamp((1 - d / 24) * 0.55 * openF, 0, 1));
      }
    if (openF > 0.62) {
      // Solid silhouette against the lit doorway - the same trick that makes
      // the sunset rex read, rather than a translucent shape that muddies.
      miniRex(15, 13, SIL, C(20, 14, 12));
      const lid = (t % 2.8) < 2.5 ? 1 : 0.15;
      px(18, 17, mix(SIL, AMBER_HOT, lid));
    }
    resetClip();

    // Gate leaves. Each is clipped to the doorway so it slides behind stone.
    const doorTop = 12;
    function leaf(x0, x1) {
      setClip(10, doorTop, 53, 31);
      rect(x0, doorTop, x1 - x0, 32 - doorTop, WOOD);
      for (let x = x0; x < x1; x += 4) {
        rect(x, doorTop, 1, 32 - doorTop, WOOD_DK);
        rect(x + 1, doorTop, 1, 32 - doorTop, WOOD_LT);
      }
      // Iron bands with rivets.
      for (const by of [doorTop + 2, 26]) {
        rect(x0, by, x1 - x0, 2, C(62, 58, 60));
        for (let x = x0 + 1; x < x1; x += 5) px(x, by + 1, C(128, 124, 126));
      }
      resetClip();
    }
    leaf(10 - gap, 32 - gap);
    leaf(32 + gap, 54 + gap);

    // Emblem straddling the seam: splits cleanly as the doors part.
    setClip(10, doorTop, 53, 31);
    for (const side of [-1, 1]) {
      const cx = 32 + side * (gap + 0.5);
      setClip(side < 0 ? 10 : 32, doorTop, side < 0 ? 31 : 53, 31);
      circle(cx - side * 0.5, 21, 7, AMBER_DIM);
      circle(cx - side * 0.5, 21, 5.6, WOOD_DK);
      // Tiny rex skull inside the medallion.
      quad(cx - 4, 20, cx + 1, 18.6, cx + 3, 20.4, cx - 3.4, 21.4, AMBER);
      quad(cx - 3.6, 21.6, cx + 2.6, 21, cx + 2.6, 22.4, cx - 3.2, 22.8, AMBER);
      px(cx + 0.6, 19.8, WOOD_DK);
    }
    resetClip();

    // Stone pillars over everything, then the lintel.
    for (const [px0, px1] of [[0, 10], [54, 64]]) {
      rect(px0, 4, px1 - px0, 28, STONE);
      for (let y = 6; y < 32; y += 5) rect(px0, y, px1 - px0, 1, STONE_DK);
      for (let y = 6; y < 32; y += 10) rect(px0 + (px1 - px0) / 2, y, 1, 5, STONE_DK);
      rect(px0 - 1, 3, px1 - px0 + 2, 2, STONE_LT);
    }
    // Banner in the park's own colours rather than the gate's timber: black
    // field, red rules, yellow lettering carrying a red drop shadow.
    rect(9, 3, 46, 10, BLACK);
    rect(9, 3, 46, 1, JP_RED);
    rect(9, 12, 46, 1, JP_RED);
    // Shadow offset sideways only. A diagonal drop shadow on a 5x7 outline
    // font lands inside the letter counters and reads as red on both sides.
    const nx = Math.round((W - textWidth('NOLAN', 1)) / 2);
    text('NOLAN', nx + 1, 4, 1, JP_RED);
    text('NOLAN', nx, 4, 1, mix(C(255, 206, 48), C(255, 238, 150), 0.35 + 0.35 * Math.sin(t * 2)));

    torch(4.5, 4, t, 3);
    torch(58.5, 4, t, 17);
  }

  // ---- 2. Welcome sign ----
  function sceneWelcome(t) {
    clear(C(5, 14, 9));
    treeline(20, JUNGLE, 4);
    treeline(26, C(4, 14, 9), 21);
    // Torch glow washing in from both edges.
    for (let y = 0; y < H; y++)
      for (let x = 0; x < W; x++) {
        const d = Math.min(Math.hypot(x - 2, y - 8), Math.hypot(x - 61, y - 8));
        pxa(x, y, JP_ORANGE, clamp((1 - d / 34) * 0.30, 0, 1));
      }

    const lines = [['WELCOME TO', 2, 1], ['NOLAN PARK', 12, 1], ['EST 2020', 23, 1]];
    lines.forEach(([s, y, sc], i) => {
      const appear = clamp((t - 0.35 - i * 0.75) / 0.5, 0, 1);
      if (appear <= 0) return;
      // Typewriter reveal, then a steady amber.
      const n = Math.ceil(appear * s.length);
      const shown = s.slice(0, n);
      const x = Math.round((W - textWidth(s, sc)) / 2);
      const col = i === 1 ? mix(AMBER, AMBER_HOT, 0.5 + 0.5 * Math.sin(t * 3)) : AMBER;
      text(shown, x, y, sc, col);
    });
    // Underline under the park name.
    const uw = clamp((t - 1.6) / 0.6, 0, 1) * 40;
    if (uw > 0) rect(32 - uw / 2, 20, uw, 1, JP_RED);
  }

  // ---- 3. T-Rex ----
  function sceneTrex(t) {
    const flash = Math.max(pulse(t, 3.9, 4.12), pulse(t, 4.28, 4.42), pulse(t, 7.4, 7.58));
    sunsetSky(27);
    // Low sun sitting on the horizon behind the tail.
    circle(50, 25, 8, mix(C(255, 200, 80), WHITE, flash * 0.6));
    circle(50, 25, 5.5, mix(C(255, 226, 150), WHITE, flash * 0.6));
    // Shallow canopy so it never swallows the legs.
    for (let x = 0; x < W; x++) {
      const top = 25 + Math.floor(hash2(x, 11) * 1.9);
      for (let y = top; y < 27; y++) px(x, y, C(9, 26, 15));
    }
    rect(0, 27, W, 5, C(5, 14, 9));

    const roar = Math.max(pulse(t, 2.1, 3.6), pulse(t, 5.6, 7.0));
    const jaw = roar * 0.62;
    const lift = roar * 0.22;
    const shake = roar > 0.4 ? Math.round(Math.sin(t * 40)) : 0;
    const bob = Math.sin(t * 2.2) * 0.7;

    const mouth = trex(3 + shake, bob + shake * 0.5, jaw, lift, t * 2.2);

    // Sound rings pushing out of the open mouth.
    if (roar > 0.15) {
      for (let k = 0; k < 3; k++) {
        const rr = 3 + ((t * 18 + k * 4.2) % 12);
        const aa = clamp((1 - rr / 15) * roar, 0, 1);
        for (let d = -0.85; d <= 0.85; d += 0.06)
          pxa(mouth[0] - Math.cos(d) * rr, mouth[1] - Math.sin(d) * rr * 0.8, AMBER_HOT, aa);
      }
    }

    // Startled birds break for the sky on the first roar.
    if (t > 2.3 && t < 6.0) {
      const f = t - 2.3;
      for (let i = 0; i < 5; i++) {
        const bx = 44 + i * 3.5 + f * (5 + hash(i * 9) * 5);
        const by = 12 - f * (2.2 + hash(i * 21) * 2.4) - i * 0.8;
        if (bx > W + 4 || by < -4) continue;
        const flap = Math.sin(f * 14 + i * 2) > 0 ? 1 : 0;
        px(bx, by, SIL); px(bx - 1, by - flap, SIL); px(bx + 1, by - flap, SIL);
      }
    }

    if (flash > 0.02) for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) pxa(x, y, WHITE, flash * 0.7);
  }

  // ---- 4. Jeep ----
  function sceneJeep(t) {
    sunsetSky(24);
    // Solid canopy band rather than scattered fronds, which read as noise.
    for (let x = 0; x < W; x++) {
      const top = 16 + Math.floor(hash2(x, 7) * 2 + hash2(Math.floor(x / 7), 23) * 3);
      for (let y = top; y < 24; y++) px(x, y, C(8, 24, 14));
    }
    for (let y = 24; y < H; y++) {
      const c = mix(C(102, 70, 40), C(58, 38, 22), (y - 24) / 8);
      for (let x = 0; x < W; x++) px(x, y, c);
    }
    for (let i = 0; i < 34; i++)
      pxa(Math.floor(hash(i * 3 + 1) * W), 24 + Math.floor(hash(i * 5 + 2) * 8), C(138, 100, 58), 0.5);

    // Something big is following: prints stomp in behind the jeep.
    let shake = 0;
    for (let i = 0; i < 4; i++) {
      const st2 = 2.4 + i * 1.05;
      if (t < st2) continue;
      const f = clamp((t - st2) / 0.28, 0, 1);
      footprint(4 + i * 15, 29, f);
      if (f < 0.5) shake = 1;
    }

    const x = -36 + ((t * 22) % 102);
    const bounce = Math.abs(Math.sin(t * 10)) > 0.7 ? 1 : 0;
    if (x < 68) jeep(x, 11 + bounce + shake, t);
  }

  // ---- 5. Blue ----
  function sceneBlue(t) {
    clear(C(3, 10, 7));
    treeline(8, C(6, 20, 12), 55);
    // Pack-mate eyes blinking in the dark behind her.
    for (let i = 0; i < 3; i++) {
      const ex = 46 + i * 6, ey = 4 + i * 3;
      const on = Math.sin(t * 1.3 + i * 2.1) > -0.2 ? 1 : 0;
      if (on) { pxa(ex, ey, C(210, 150, 30), 0.75); pxa(ex + 2, ey, C(210, 150, 30), 0.75); }
    }

    const blink = (t % 3.4) > 3.15 ? 1 : 0;
    const snap = Math.max(pulse(t, 2.4, 3.0), pulse(t, 5.1, 5.6));
    const bob = Math.sin(t * 1.6) * 1.2;
    raptorHead(7, 7 + bob, t, blink, snap * 0.32, C(88, 124, 80), C(46, 116, 210));

    // Breath in the cool air.
    for (let i = 0; i < 6; i++) {
      const ph = (t * 0.55 + i * 0.16) % 1;
      pxa(56 + ph * 7, 13 + bob + Math.sin(ph * 6 + i) * 2 - ph * 2, C(150, 175, 190), (1 - ph) * 0.30);
    }
  }

  // ---- 6. Hatching egg ----
  function sceneEgg(t) {
    clear(C(8, 5, 12));
    // Incubator lamp.
    for (let y = 0; y < H; y++)
      for (let x = 0; x < W; x++) {
        const d = Math.hypot((x - 32) * 0.8, y - 20);
        pxa(x, y, JP_ORANGE, clamp((1 - d / 30) * 0.5, 0, 1));
      }
    rect(0, 29, W, 3, C(52, 34, 18));
    for (let x = 0; x < W; x += 3) px(x, 29, C(76, 52, 28));

    const wob = t < 3.0 ? Math.sin(t * 9) * (0.6 + t * 0.35) : 0;
    const capF = smooth((t - 3.0) / 0.7);
    const headF = smooth((t - 3.4) / 1.3);

    // Shell.
    const ex = 32 + wob, ey = 19;
    ell(ex, ey, 8.5, 11, C(238, 226, 198));
    ell(ex - 2.5, ey - 3, 4.5, 6, C(250, 242, 222));
    for (let i = 0; i < 22; i++) {
      const sx = ex - 7 + hash(i * 13) * 14;
      const sy = ey - 9 + hash(i * 29) * 18;
      if (Math.hypot((sx - ex) / 8.5, (sy - ey) / 11) < 0.92) pxa(sx, sy, C(196, 176, 142), 0.6);
    }

    // Baby raptor pushing up out of the shell.
    if (headF > 0) {
      const hy = ey - 6 - headF * 7;
      ell(ex, hy, 4.2, 3.6, C(112, 138, 92));
      quad(ex + 2, hy - 1, ex + 8, hy + 0.6, ex + 8, hy + 2.4, ex + 2, hy + 2.6, C(112, 138, 92));
      line(ex - 3, hy - 3, ex + 4, hy - 4.2, C(52, 120, 206), 2, 1.6);
      circle(ex + 1.4, hy - 0.6, 1.6, C(255, 214, 60));
      px(ex + 1.4, hy - 0.6, C(16, 10, 6));
      px(ex + 2.1, hy - 1.3, WHITE);
      px(ex + 7, hy + 1, C(60, 74, 48));
      // Chirp.
      const ch = pulse(t % 1.6, 0.1, 0.45);
      if (ch > 0.1) for (let k = 1; k <= 2; k++) {
        const rr = 3 + k * 2.2;
        for (let d = -0.6; d <= 0.6; d += 0.12)
          pxa(ex + 8 + Math.cos(d) * rr, hy + 1 + Math.sin(d) * rr, AMBER_HOT, ch * 0.7);
      }
    }

    // Cracks, then the cap tips off to the right.
    if (t > 1.4) {
      const cf = clamp((t - 1.4) / 1.6, 0, 1);
      for (let i = 0; i < 7; i++) {
        if (i / 7 > cf) break;
        const a = -2.4 + i * 0.34;
        const x0 = ex + Math.cos(a) * 7, y0 = ey + Math.sin(a) * 9;
        line(x0, y0, x0 + Math.cos(a + 1.4) * 2.4, y0 + Math.sin(a + 1.4) * 2.4, C(120, 104, 78), 1);
      }
    }
    if (capF > 0) {
      const cx2 = ex + 4 + capF * 12, cy2 = ey - 9 - Math.sin(capF * Math.PI) * 7 + capF * 10;
      const [rx1, ry1] = rot(cx2 - 4, cy2, cx2, cy2, capF * 2.4);
      const [rx2, ry2] = rot(cx2 + 4, cy2, cx2, cy2, capF * 2.4);
      line(rx1, ry1, rx2, ry2, C(238, 226, 198), 3, 3);
    }

    for (let i = 0; i < 10; i++) {
      const ph = (t * 0.4 + hash(i * 7)) % 1;
      pxa(12 + hash(i * 11) * 40, 28 - ph * 22, AMBER_HOT, (1 - ph) * 0.45);
    }
  }

  // ---- 7. The emblem ----
  function sceneLogo(t) {
    clear(C(3, 3, 6));
    const grow = smooth(t / 0.9);
    const RX = 15 * grow, RY = 11.5 * grow;
    if (RX < 2) return;

    ell(32, 12, RX, RY, BLACK);
    for (let y = 0; y <= 26; y++)
      for (let x = 16; x <= 48; x++) {
        if (Math.hypot((x - 32) / (RX - 1.8), (y - 12) / (RY - 1.8)) > 1) continue;
        px(x, y, mix(C(248, 150, 34), C(214, 58, 26), y / 22));
      }

    if (grow > 0.9) miniRex(19, 2, BLACK, C(86, 20, 16));

    // Name band across the bottom.
    rect(1, 25, 62, 7, BLACK);
    rect(1, 25, 62, 1, C(72, 56, 34));
    textCentered('NOLAN PARK', 25, 1, AMBER);

    // Shimmer sweep across the disc.
    if (t > 1.2) {
      const sw = ((t - 1.2) * 32) % 112;
      for (let y = 0; y < 25; y++) {
        const x = sw - 24 + y * 0.7;
        for (let k = 0; k < 3; k++)
          if (Math.hypot((x + k - 32) / (RX - 2), (y - 12) / (RY - 2)) < 1)
            pxa(x + k, y, WHITE, 0.20 - k * 0.055);
      }
    }
  }

  // ---- 8. Happy birthday ----
  function sceneBirthday(t) {
    clear(C(6, 4, 10));

    // Fireworks: a burst every 0.8s from a hashed position.
    for (let bidx = 0; bidx < 14; bidx++) {
      const bt = bidx * 0.8;
      const age = t - bt;
      if (age < 0 || age > 1.5) continue;
      const bx = 8 + hash(bidx * 17 + 3) * 48;
      const by = 5 + hash(bidx * 29 + 11) * 18;
      const hue = [AMBER, C(90, 220, 140), C(90, 170, 255), JP_RED, C(255, 120, 200)][bidx % 5];
      for (let p = 0; p < 12; p++) {
        const a = (p / 12) * Math.PI * 2 + hash(bidx * 7) * 6;
        const sp = 7 + hash(bidx * 13 + p) * 5;
        const d = age * sp;
        const fx = bx + Math.cos(a) * d;
        const fy = by + Math.sin(a) * d * 0.85 + age * age * 6;
        pxa(fx, fy, hue, clamp(1 - age / 1.5, 0, 1) * 0.95);
      }
    }

    if (t < 4.0) {
      // Scrolling marquee.
      const s = 'HAPPY BIRTHDAY';
      const w = textWidth(s, 2);
      const x = W - ((t * 30) % (w + W));
      text(s, x, 9, 2, AMBER);
    } else {
      const f = t - 4.0;
      const c1 = mix(AMBER, AMBER_HOT, 0.5 + 0.5 * Math.sin(f * 4));
      const c2 = mix(C(90, 220, 140), WHITE, 0.5 + 0.5 * Math.sin(f * 4 + 1.6));
      textCentered('NOLAN', 2, 2, c1);
      textCentered('IS 6!', 17, 2, c2);
      // Two tiny raptors sprinting along the bottom.
      for (let i = 0; i < 2; i++) {
        const rx = ((f * 17 + i * 33) % 78) - 8;
        const ry = 31 - Math.abs(Math.sin(f * 9 + i)) * 1;
        ell(rx, ry - 2, 2.4, 1.4, C(70, 190, 120));
        line(rx + 2, ry - 2.5, rx + 5, ry - 3.4, C(70, 190, 120), 1.4, 1);
        line(rx - 2, ry - 2, rx - 5, ry - 3.6, C(70, 190, 120), 1.4, 1);
        px(rx + 4.6, ry - 3.6, C(255, 220, 90));
      }
    }

    // Marquee bulb border, ties back to the everyday sign.
    const bulb = mix(AMBER, AMBER_HOT, 0.5 + 0.5 * Math.sin(t * 6));
    for (let x = 1; x < W - 1; x += 5) { px(x, 0, bulb); px(x, H - 1, bulb); }
    for (let y = 1; y < H - 2; y += 5) { px(0, y, bulb); px(W - 1, y, bulb); }
  }

  const SCENES = [
    { key: 'gate',     name: 'The Gates',     dur: 9.0,  fn: sceneGate },
    { key: 'welcome',  name: 'Welcome Sign',  dur: 6.0,  fn: sceneWelcome },
    { key: 'trex',     name: 'T-Rex',         dur: 9.0,  fn: sceneTrex },
    { key: 'jeep',     name: 'Jeep 10',       dur: 7.0,  fn: sceneJeep },
    { key: 'blue',     name: 'Blue',          dur: 7.0,  fn: sceneBlue },
    { key: 'egg',      name: 'Hatching Egg',  dur: 8.0,  fn: sceneEgg },
    { key: 'logo',     name: 'Park Emblem',   dur: 6.0,  fn: sceneLogo },
    { key: 'birthday', name: 'Nolan Is 6',    dur: 11.0, fn: sceneBirthday },
  ];

  const api = { SCENES, byKey: (k) => SCENES.find((s) => s.key === k) };
  global.JP = api;
  if (typeof module !== 'undefined') module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
