// Writes unscaled, unquantized frames so the C++ port can be diffed
// against the simulator frame by frame.
const fs = require('fs'), path = require('path');
const BS = require('./engine.js');
BS.setFont(require('./font.js'));
globalThis.BS = BS;
const JP = require('./scenes.js');
const LAB = require('./lab.js');
const key = process.argv[2], frames = parseInt(process.argv[3], 10), out = process.argv[4];
// "lab" and "labmsg" are the lab face, idle and carrying a message; the
// message must match LAB_MSG in host/main.cpp.
const LAB_MSG = 'BACK IN 10 MIN';
const sc = key === 'lab' ? { dur: LAB.IDLE_CYCLE, fn: (t) => LAB.frame(t, false, '') }
  : key === 'labmsg' ? { dur: LAB.msgCycle(LAB_MSG), fn: (t) => LAB.frame(t, true, LAB_MSG) }
  : JP.byKey(key);
fs.mkdirSync(out, { recursive: true });
for (let f = 0; f < frames; f++) {
  BS.resetClip(); BS.clear(0);
  sc.fn((f / frames) * sc.dur);
  const h = Buffer.from('P6\n64 32\n255\n');
  fs.writeFileSync(path.join(out, `${key}_${String(f).padStart(2, '0')}.ppm`),
                   Buffer.concat([h, Buffer.from(BS.buf)]));
}
