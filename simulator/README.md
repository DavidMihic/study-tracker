# UI demo (temporary)

The real firmware compiled to WebAssembly, so the device can be tried in a web browser, plus a design
brief for whoever helps with the UI. Page and brief are in Croatian.

| Output | What it is |
|---|---|
| `dist/study-tracker-demo.html` | the demo, one self-contained file (works offline) |
| `dist/dizajn-brief.pdf` | constraints, colours, fonts and pictures of all screens |

Send both files. The HTML is downloaded and opened in a browser (double-click). `dist/` is not in git.

## Setup on a new Windows machine

Verified on the PC this was made on (Emscripten 7.0.1). Steps marked * were already in place there and
have not been tried from a clean machine.

1. \*Git for Windows (it includes **Git Bash**, which runs the build script): <https://git-scm.com>
2. \*scoop, in PowerShell (not as administrator):

   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression
   ```

3. Emscripten, in PowerShell. The download is about 800 MB:

   ```powershell
   scoop install emscripten
   cd "$HOME\scoop\apps\emscripten\current"
   .\emsdk.bat install latest
   .\emsdk.bat activate latest
   ```

   Node.js and Python do not need to be installed separately; Emscripten brings its own.
4. \*The Arduino libraries. They are read from `software/.pio/libdeps/feather/`, which is not in git, so
   build the firmware once: open `software/` in VS Code with the PlatformIO extension and press Build, or
   run this in a PlatformIO terminal:

   ```
   cd software
   pio run -e feather
   ```

5. \*Microsoft Edge (part of Windows) or Chrome, only for the PDF.

## Build

In **Git Bash**:

```bash
cd simulator
./build.sh           # the demo
./build.sh --brief   # the demo and the PDF
```

Rebuild after changing the UI in `software/src/`; send the new files.

The script finds Emscripten if `emcc` is on the PATH, or through `$EMSDK`, the scoop folder or `~/emsdk`.

## If something fails

| Message | Fix |
|---|---|
| `emcc not found` | step 3, including `activate`; or set `EMSDK` to the emsdk folder |
| `Arduino libraries not found` | step 4 |
| `no Edge or Chrome found` | `BROWSER_EXE="C:/path/to/browser.exe" ./build.sh --brief` |
| `$'\r': command not found` | `build.sh` got Windows line endings; save it with LF |
| compile error after a firmware change | the firmware now uses something the replacements below lack; add it in `shim/` or `src/` |

## How it works

`software/src/app`, `ui`, `data` and most drivers are compiled unchanged. Replaced here:

| Real | Replacement |
|---|---|
| Arduino core, SPI, ST7789, RTClib | `shim/` (the real Adafruit GFX and u8g2 draw into a frame buffer) |
| `drivers/encoder`, `rtc_clock`, `flash_store` | `src/sim_*.cpp` (data is kept in the browser's localStorage) |
| `main.cpp` | `src/sim_main.cpp` (mirror changes there) |

- `web/page.html`: the demo page.
- `brief/brief.html`: the text of the brief. Colours and screen pictures are filled in at build time. The
  fonts table and the screen flow are written by hand.
- `brief/screens.mjs`: clicks through the menus to take the pictures. If a menu is rearranged, update the
  key sequence there and check the pictures in the PDF.
- Demo study data comes from `software/src/debug/`. If that folder is deleted, the demo starts empty and
  the brief's statistics and subject pictures have no data.

Differences from the device: redrawing is instant, there is no power switch, and monitor colours and size
differ from the 2" panel.

## Removing it

Delete the `simulator/` folder. Nothing outside it refers to it. Optionally `scoop uninstall emscripten`,
and clear the browser's site data if the demo's saved state matters.
