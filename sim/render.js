/*
 * Headless contact-sheet renderer. Samples frames across a scene's
 * duration and writes a scaled PPM grid so the art can be reviewed
 * without flashing the panel.
 *
 *   node render.js <sceneKey|all> [frames] [scale]
 */
const fs = require('fs');
const path = require('path');

const BS = require('./engine.js');
BS.setFont(require('./font.js'));
globalThis.BS = BS;
const JP = require('./scenes.js');
const LAB = require('./lab.js');

// The lab face, idle and carrying a message, rendered like scenes.
const LAB_MSG = process.env.LAB_MSG || 'BACK IN 10 MIN';
const LAB_FACES = [
  { key: 'lab', name: 'Lab sign, idle', dur: LAB.IDLE_CYCLE, fn: (t) => LAB.frame(t, false, '') },
  { key: 'labmsg', name: 'Lab sign, message', dur: LAB.msgCycle(LAB_MSG), fn: (t) => LAB.frame(t, true, LAB_MSG) },
];
const byKey = (k) => JP.byKey(k) || LAB_FACES.find((s) => s.key === k);

const W = BS.W, H = BS.H;

// Protomatter runs at 4-bit depth over a 565 framebuffer. Quantize the
// preview the same way so the simulator does not flatter the panel.
function quant(v, bits) {
  const levels = (1 << bits) - 1;
  return Math.round(Math.round((v / 255) * levels) / levels * 255);
}

function renderFrame(scene, t) {
  BS.resetClip();
  BS.clear(0);
  scene.fn(t);
  const out = new Uint8Array(W * H * 3);
  for (let i = 0; i < W * H; i++) {
    out[i * 3] = quant(quant(BS.buf[i * 3], 5), 4);
    out[i * 3 + 1] = quant(quant(BS.buf[i * 3 + 1], 6), 4);
    out[i * 3 + 2] = quant(quant(BS.buf[i * 3 + 2], 5), 4);
  }
  return out;
}

function sheet(scene, frames, scale, cols) {
  const rows = Math.ceil(frames / cols);
  const gut = 3;
  const cw = W * scale + gut, chh = H * scale + gut;
  const outW = cols * cw + gut, outH = rows * chh + gut;
  const img = new Uint8Array(outW * outH * 3);
  img.fill(40);

  for (let f = 0; f < frames; f++) {
    const t = (f / frames) * scene.dur;
    const fb = renderFrame(scene, t);
    const ox = gut + (f % cols) * cw, oy = gut + Math.floor(f / cols) * chh;
    for (let y = 0; y < H * scale; y++) {
      for (let x = 0; x < W * scale; x++) {
        const sx = Math.floor(x / scale), sy = Math.floor(y / scale);
        const si = (sy * W + sx) * 3;
        const di = ((oy + y) * outW + (ox + x)) * 3;
        img[di] = fb[si]; img[di + 1] = fb[si + 1]; img[di + 2] = fb[si + 2];
      }
    }
  }
  return { img, outW, outH };
}

const key = process.argv[2] || 'all';
const frames = parseInt(process.argv[3] || '8', 10);
const scale = parseInt(process.argv[4] || '6', 10);
const outDir = process.env.OUT_DIR || '/tmp/jpshots';
fs.mkdirSync(outDir, { recursive: true });

const TIMES = process.env.TIMES ? process.env.TIMES.split(',').map(Number) : null;

function sheetAt(scene, times, scale, cols) {
  const rows = Math.ceil(times.length / cols);
  const gut = 3;
  const cw = W * scale + gut, chh = H * scale + gut;
  const outW = cols * cw + gut, outH = rows * chh + gut;
  const img = new Uint8Array(outW * outH * 3);
  img.fill(40);
  times.forEach((t, f) => {
    const fb = renderFrame(scene, t);
    const ox = gut + (f % cols) * cw, oy = gut + Math.floor(f / cols) * chh;
    for (let y = 0; y < H * scale; y++)
      for (let x = 0; x < W * scale; x++) {
        const si = (Math.floor(y / scale) * W + Math.floor(x / scale)) * 3;
        const di = ((oy + y) * outW + (ox + x)) * 3;
        img[di] = fb[si]; img[di + 1] = fb[si + 1]; img[di + 2] = fb[si + 2];
      }
  });
  return { img, outW, outH };
}

const list = key === 'all' ? JP.SCENES.concat(LAB_FACES) : [byKey(key)];
for (const scene of list) {
  if (!scene) { console.error('no such scene:', key); process.exit(1); }
  const { img, outW, outH } = TIMES
    ? sheetAt(scene, TIMES, scale, Math.min(TIMES.length, 3))
    : sheet(scene, frames, scale, 4);
  const file = path.join(outDir, scene.key + '.ppm');
  const header = Buffer.from(`P6\n${outW} ${outH}\n255\n`);
  fs.writeFileSync(file, Buffer.concat([header, Buffer.from(img)]));
  console.log(file, outW + 'x' + outH, scene.name);
}

// Ad-hoc: TIMES=1.0,2.8,3.2 node render.js <key> renders those exact moments.
