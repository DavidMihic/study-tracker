// Drives the simulator in Node with made-up time and input, and returns a picture of every screen for the
// design brief. The key sequences follow the menus in software/src/app/screens: if a menu is rearranged,
// adjust them here and look at the pictures.
import { createRequire } from "node:module";
import { resolve } from "node:path";
import { deflateSync } from "node:zlib";

const START = Date.UTC(2026, 10, 26, 18, 42, 0) / 1000;   // fixed date, so the pictures do not change by themselves

async function boot(corePath, storage, clock, seed) {
  const create = createRequire(import.meta.url)(resolve(corePath));
  const core = await create({
    storageLoad: (name) => storage[name] ?? null,
    storageSave: (name, hex) => { storage[name] = hex; },
  });
  let time = 0;
  const run = (ms) => { for (let t = 0; t < ms; t += 5) core._sim_loop((time += 5)); };
  core._sim_setup(time, clock, seed ? 1 : 0);
  run(200);
  return {
    core,
    run,
    turn(clicks) { for (let i = 0; i < Math.abs(clicks); i++) { core._sim_turn(Math.sign(clicks)); run(20); } run(50); },
    click() { core._sim_button(1); run(60); core._sim_button(0); run(100); },
    hold() { core._sim_button(1); run(1000); core._sim_button(0); run(100); },
    skip(seconds) { core._sim_skip(seconds); run(1200); },
  };
}

export async function captureScreens(corePath) {
  const storage = {};
  let device = await boot(corePath, storage, START, true);
  const shots = {};
  const shot = (name) => {
    const start = device.core._sim_frame() >> 1;
    shots[name] = png(device.core.HEAPU16.slice(start, start + 320 * 240), 320, 240, 4);
  };
  const turn = (n) => device.turn(n), click = () => device.click(), run = (ms) => device.run(ms);

  shot("home");
  click(); shot("homeMenu");

  // Subject picker
  turn(1); click(); shot("picker");
  turn(300); shot("pickerEnd");
  turn(-2); click(); shot("electives");             // third row from the end: the elective folder
  turn(-300); click();                              // top Back row (or the only subject if the list is short)
  turn(300); turn(-1); click(); shot("years");      // second row from the end: another year / semester
  turn(1); click(); shot("half");                   // the next year
  turn(1); click();                                 // its summer semester
  click(); shot("switchSemester");
  turn(1); run(100); click();                       // "Ne": choose the subject only
  run(200);

  // Back to a subject of the current semester, then study.
  click(); turn(1); click(); turn(-300); click(); run(200);
  click(); click(); run(1500); device.skip(7 * 60); shot("studying");
  click(); shot("studyingMenu");
  click(); run(1200); shot("paused");
  click(); click(); run(200);                       // resume
  device.skip(19 * 60); run(500); shot("targetReached");

  // Power cut during the block: a second device starts from the saved data, five minutes later.
  click(); click(); run(200); click(); click(); run(200);   // pause and resume, which saves the block
  const before = device;
  device = await boot(corePath, { ...storage }, before.core._sim_clock() + 5 * 60, false);
  run(1200); shot("restored");
  device = before;

  click(); turn(1); click(); run(1500); shot("break");
  click(); shot("breakMenu");
  turn(1); click(); run(300);                       // home

  // Statistics
  click(); turn(2); click(); run(300); shot("statsToday");
  turn(1); run(300); shot("statsWeek");
  click(); run(200);

  // Settings
  click(); turn(3); click(); run(200); shot("settings");
  click(); turn(1); shot("settingsEdit");
  turn(-1); click();
  turn(3); click(); run(200); shot("sound");
  turn(10); click(); run(200);                      // last row: back
  turn(1); click(); run(200); shot("clockSet");
  turn(10); click(); run(200);                      // last row: cancel
  turn(1); click(); run(200); shot("resetMenu");
  turn(1); click(); run(200); shot("resetConfirm");
  click(); run(200);                                // "Natrag" is preselected
  turn(10); click(); run(200); turn(10); click(); run(200);   // back to settings, then home

  device.core._sim_screensaver(); run(2800); shot("screensaver");
  return shots;
}

// The checksum of a PNG chunk (zlib.crc32 only exists in newer Node versions).
function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ (0xedb88320 & -(crc & 1));
  }
  return (crc ^ 0xffffffff) >>> 0;
}

// A PNG of an RGB565 frame, enlarged without smoothing.
function png(frame, width, height, scale) {
  const w = width * scale, h = height * scale;
  const raw = Buffer.alloc((w * 3 + 1) * h);        // each row starts with filter type 0
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const c = frame[Math.floor(y / scale) * width + Math.floor(x / scale)];
      const r = c >> 11, g = (c >> 5) & 63, b = c & 31;
      const at = y * (w * 3 + 1) + 1 + x * 3;
      raw[at] = (r << 3) | (r >> 2);
      raw[at + 1] = (g << 2) | (g >> 4);
      raw[at + 2] = (b << 3) | (b >> 2);
    }
  }
  const chunk = (type, data) => {
    const body = Buffer.concat([Buffer.from(type), data]);
    const out = Buffer.alloc(body.length + 8);
    out.writeUInt32BE(data.length, 0);
    body.copy(out, 4);
    out.writeUInt32BE(crc32(body), body.length + 4);
    return out;
  };
  const header = Buffer.alloc(13);
  header.writeUInt32BE(w, 0);
  header.writeUInt32BE(h, 4);
  header.set([8, 2, 0, 0, 0], 8);                   // 8 bits per channel, RGB
  return Buffer.concat([
    Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]),
    chunk("IHDR", header), chunk("IDAT", deflateSync(raw)), chunk("IEND", Buffer.alloc(0)),
  ]);
}
