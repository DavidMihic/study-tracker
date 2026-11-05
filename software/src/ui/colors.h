// The colour palette (RGB565, the format the display uses). Change a colour here, not in the screens.
// Plain C++ (no Arduino), so it can be included anywhere.
#pragma once
#include <stdint.h>

namespace Colors {

constexpr uint16_t kBackground = 0x0000;   // black
constexpr uint16_t kText       = 0xFFFF;   // white
constexpr uint16_t kMuted      = 0x8410;   // grey, for hints
constexpr uint16_t kAccent     = 0x07FF;   // cyan
constexpr uint16_t kGood       = 0x07E0;   // green
constexpr uint16_t kWarning    = 0xFFE0;   // yellow
constexpr uint16_t kBad        = 0xF800;   // red
constexpr uint16_t kSelection  = 0x0014;   // dark blue behind the selected list row
constexpr uint16_t kCard       = 0x2104;   // dark grey card in the pop-up menus

}  // namespace Colors
