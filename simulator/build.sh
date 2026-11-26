#!/usr/bin/env bash
# Builds the browser demo (dist/study-tracker-demo.html) and, with "--brief", the design brief
# (dist/dizajn-brief.pdf). Run from Git Bash: ./build.sh   or   ./build.sh --brief
# Only reads from ../software; everything it writes is inside this folder.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
sw="$here/../software"
libs="$sw/.pio/libdeps/feather"
gfx="$libs/Adafruit GFX Library"
u8g2="$libs/U8g2_for_Adafruit_GFX/src"
build="$here/build"
dist="$here/dist"

fail() { echo "ERROR: $*" >&2; exit 1; }

# Emscripten is usually not on the PATH: load its environment from $EMSDK or from the scoop install.
if ! command -v emcc >/dev/null; then
  for sdk in "${EMSDK:-}" "$HOME/scoop/apps/emscripten/current" "$HOME/emsdk"; do
    if [ -n "$sdk" ] && [ -f "$sdk/emsdk_env.sh" ]; then
      set +u
      EMSDK_QUIET=1 source "$sdk/emsdk_env.sh"
      set -u
      break
    fi
  done
fi
command -v emcc >/dev/null || fail "emcc not found. Install and activate Emscripten (see README.md)."
command -v node >/dev/null || fail "node not found. Install Node.js (see README.md)."
[ -f "$gfx/Adafruit_GFX.cpp" ] && [ -f "$u8g2/U8g2_for_Adafruit_GFX.cpp" ] ||
  fail "Arduino libraries not found in $libs. Run 'pio run -e feather' once in software/ (see README.md)."

mkdir -p "$build" "$dist"

includes=(-I"$here/shim" -I"$here/src" -I"$sw/src" -I"$gfx" -I"$u8g2")
flags=(-Oz -Uunix -DARDUINO=10819 -DDUMMY_DATA -ffunction-sections -fdata-sections)

# The firmware: everything except main.cpp and the drivers that need real hardware.
sources=()
while IFS= read -r file; do sources+=("$file"); done < <(find "$sw/src/app" "$sw/src/data" "$sw/src/ui" -name '*.cpp' | sort)
if [ -d "$sw/src/debug" ]; then
  while IFS= read -r file; do sources+=("$file"); done < <(find "$sw/src/debug" -name '*.cpp' | sort)
fi
for driver in display text_field input button buzzer backlight; do sources+=("$sw/src/drivers/$driver.cpp"); done
sources+=("$here"/src/*.cpp "$gfx/Adafruit_GFX.cpp" "$u8g2/U8g2_for_Adafruit_GFX.cpp")

echo "Compiling fonts..."
emcc "${flags[@]}" -I"$u8g2" -c "$u8g2/u8g2_fonts.c" -o "$build/fonts.o"

echo "Compiling firmware..."
em++ "${flags[@]}" -std=gnu++17 "${includes[@]}" "${sources[@]}" "$build/fonts.o" -o "$build/core.js" \
  -Wl,--gc-sections \
  -sSINGLE_FILE=1 -sMODULARIZE=1 -sEXPORT_NAME=createStudyTracker -sENVIRONMENT=web,node \
  -sFILESYSTEM=0 -sSTACK_SIZE=262144 -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16

node "$here/web/assemble.mjs" "$build/core.js" "$dist/study-tracker-demo.html"
echo "Demo:  $dist/study-tracker-demo.html"

if [ "${1:-}" = "--brief" ]; then
  node "$here/brief/make_brief.mjs" "$build/core.js" "$sw/src" "$build" "$dist/dizajn-brief.pdf"
fi
