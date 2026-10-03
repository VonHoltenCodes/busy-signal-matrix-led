// Writes unscaled, unquantized frames so the C++ port can be diffed
// against the simulator frame by frame.
const fs = require('fs'), path = require('path');
const BS = require('./engine.js');
BS.setFont(require('./font.js'));
globalThis.BS = BS;
const JP = require('./scenes.js');
const key = process.argv[2], frames = parseInt(process.argv[3], 10), out = process.argv[4];
const sc = JP.byKey(key);
fs.mkdirSync(out, { recursive: true });
for (let f = 0; f < frames; f++) {
  BS.resetClip(); BS.clear(0);
  sc.fn((f / frames) * sc.dur);
  const h = Buffer.from('P6\n64 32\n255\n');
  fs.writeFileSync(path.join(out, `${key}_${String(f).padStart(2, '0')}.ppm`),
                   Buffer.concat([h, Buffer.from(BS.buf)]));
}
