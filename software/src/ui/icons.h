// Small pictograms for the pop-up cards, drawn with simple shapes (no bitmaps to maintain).
// Each icon fits in a 32 x 32 square around its centre point.
#pragma once
#include <stdint.h>

namespace Icons {

enum class Icon : uint8_t {
  Play,       // triangle: start / resume
  Pause,      // two bars
  Stop,       // square
  Gear,       // settings
  Book,       // choose a subject
  Home,       // back to the home screen
  Check,      // yes
  Cross,      // no
};

// color = the icon; background = what is behind it (the gear's hole and the book's pages use it).
void draw(Icon icon, int16_t centerX, int16_t centerY, uint16_t color, uint16_t background);

}  // namespace Icons
