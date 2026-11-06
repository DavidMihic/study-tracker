#!/usr/bin/env bash
# Builds and runs the subject-name width check. Needs g++ and gcc on the PATH and one normal
# `pio run -e feather` first (that downloads the u8g2 library into .pio/libdeps).
set -e
here="$(cd "$(dirname "$0")" && pwd)"
sw="$here/../.."
lib="$sw/.pio/libdeps/feather/U8g2_for_Adafruit_GFX/src"
out="$(mktemp -d)"

gcc -c "$lib/u8g2_fonts.c" -I"$lib" -o "$out/fonts.o"
g++ -std=gnu++17 -I"$here/stub" -I"$lib" -I"$sw/src" \
    "$here/name_widths.cpp" "$sw/src/data/subjects.cpp" "$lib/U8g2_for_Adafruit_GFX.cpp" "$out/fonts.o" \
    -o "$out/name_widths"
"$out/name_widths" "$@"
rm -rf "$out"
