// Puts the compiled firmware (core.js) into page.html, giving one self-contained file.
import { readFileSync, writeFileSync } from "node:fs";

const [corePath, outPath] = process.argv.slice(2);
const page = readFileSync(new URL("./page.html", import.meta.url), "utf8");
// "</script" inside the code would end the script element early.
const core = readFileSync(corePath, "utf8").replace(/<\/script/gi, "<\\/script");
// A function as the replacement, so "$" sequences in the code are not treated as patterns.
writeFileSync(outPath, page.replace("/*CORE*/", () => core));
