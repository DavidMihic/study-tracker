// Small pictograms for the pop-up cards and list rows, drawn with simple shapes (no bitmaps to maintain).
// Card icons fit in a 32 x 32 square around their centre point; Star and Return are small (about 16 px)
// and are meant for list rows.
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
  Snowflake,  // winter semester
  Sun,        // summer semester
  Star,       // favourite (small)
  Return,     // "opens a list": arrow going down and then right (small)
  Back,       // triangle pointing left, card size
  BackSmall,  // triangle pointing left, list-row size
  Chart,      // three bars: statistics
};

// color = the icon; background = what is behind it (the gear's hole and the book's pages use it).
void draw(Icon icon, int16_t centerX, int16_t centerY, uint16_t color, uint16_t background);

}  // namespace Icons
