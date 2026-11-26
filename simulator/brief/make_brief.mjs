// Makes the design brief: screen pictures from the simulator + colours from ui/colors.h are put into
// brief.html, which headless Microsoft Edge (or Chrome) prints to PDF.
// Usage: node make_brief.mjs <core.js> <software/src> <work folder> <out.pdf>
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { join, resolve } from "node:path";
import { pathToFileURL } from "node:url";
import { captureScreens } from "./screens.mjs";

const [corePath, sourceDir, workDir, pdfPath] = process.argv.slice(2).map((p) => resolve(p));

const shots = await captureScreens(corePath);
const shotDir = join(workDir, "screens");
mkdirSync(shotDir, { recursive: true });
for (const [name, bytes] of Object.entries(shots)) writeFileSync(join(shotDir, `${name}.png`), bytes);

// What each colour is used for. A colour added to colors.h later is still listed, under its code name.
const uses = {
  kBackground: "pozadina", kText: "glavni tekst", kMuted: "sporedni tekst i upute",
  kAccent: "naslovi, sat i štoperica", kGood: "pozitivno: cilj postignut, datum, EKG linija",
  kWarning: "upozorenja, pauza, omiljeni predmet", kBad: "greške", kSelection: "označeni red ili kartica",
  kDim: "male prigušene ikone", kCard: "kartice u izborniku",
};
const swatches = [...readFileSync(join(sourceDir, "ui", "colors.h"), "utf8").matchAll(/(k\w+)\s*=\s*0x([0-9A-Fa-f]{4})/g)]
  .map(([, name, hex]) => {
    const c = parseInt(hex, 16), r = c >> 11, g = (c >> 5) & 63, b = c & 31;
    const rgb = "#" + [(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)]
      .map((v) => v.toString(16).padStart(2, "0")).join("").toUpperCase();
    return `<div class="swatch"><span style="background:${rgb}"></span><div><b>${rgb}</b><br>${uses[name] ?? name}</div></div>`;
  }).join("\n");

const html = readFileSync(new URL("./brief.html", import.meta.url), "utf8")
  .replace("{{colors}}", () => swatches)
  .replace(/\{\{screen:(\w+)\}\}/g, (_, name) => {
    if (!shots[name]) throw new Error(`brief.html asks for an unknown screen: ${name}`);
    return `data:image/png;base64,${shots[name].toString("base64")}`;
  });
const htmlPath = join(workDir, "brief.html");
writeFileSync(htmlPath, html);

const browser = [
  process.env.BROWSER_EXE,
  `${process.env["ProgramFiles(x86)"]}\\Microsoft\\Edge\\Application\\msedge.exe`,
  `${process.env.ProgramFiles}\\Microsoft\\Edge\\Application\\msedge.exe`,
  `${process.env.ProgramFiles}\\Google\\Chrome\\Application\\chrome.exe`,
].find((path) => path && existsSync(path));
if (!browser) {
  console.error(`ERROR: no Edge or Chrome found (set BROWSER_EXE). The brief is at ${htmlPath}; print it by hand.`);
  process.exit(1);
}
rmSync(pdfPath, { force: true });
execFileSync(browser, [
  "--headless", "--disable-gpu", "--no-pdf-header-footer",
  `--user-data-dir=${join(workDir, "browser-profile")}`,
  `--print-to-pdf=${pdfPath}`, pathToFileURL(htmlPath).href,
], { stdio: "ignore", timeout: 60000 });
if (!existsSync(pdfPath)) {
  console.error("ERROR: the browser did not write the PDF.");
  process.exit(1);
}
console.log(`Brief: ${pdfPath}`);
