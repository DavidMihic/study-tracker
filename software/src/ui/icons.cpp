#include "ui/icons.h"
#include "drivers/display.h"

namespace {

// A line drawn 3 pixels thick.
void thickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  Adafruit_ST7789& t = Display::tft();
  t.drawLine(x0, y0, x1, y1, color);
  t.drawLine(x0 + 1, y0, x1 + 1, y1, color);
  t.drawLine(x0, y0 + 1, x1, y1 + 1, color);
}

// Eight tooth positions around the gear (about 12 px from the centre, every 45 degrees).
const int8_t kTeeth[8][2] = {{12, 0}, {8, 8}, {0, 12}, {-8, 8}, {-12, 0}, {-8, -8}, {0, -12}, {8, -8}};

}  // namespace

namespace Icons {

void draw(Icon icon, int16_t cx, int16_t cy, uint16_t color, uint16_t background) {
  Adafruit_ST7789& t = Display::tft();
  switch (icon) {
    case Icon::Play:
      t.fillTriangle(cx - 8, cy - 13, cx - 8, cy + 13, cx + 13, cy, color);
      break;

    case Icon::Pause:
      t.fillRect(cx - 11, cy - 13, 8, 26, color);
      t.fillRect(cx + 3, cy - 13, 8, 26, color);
      break;

    case Icon::Stop:
      t.fillRect(cx - 12, cy - 12, 24, 24, color);
      break;

    case Icon::Gear:
      for (const auto& tooth : kTeeth) t.fillCircle(cx + tooth[0], cy + tooth[1], 4, color);
      t.fillCircle(cx, cy, 10, color);
      t.fillCircle(cx, cy, 4, background);   // the hole
      break;

    case Icon::Book:
      t.fillRect(cx - 12, cy - 14, 24, 28, color);         // cover
      t.fillRect(cx - 7, cy - 11, 17, 22, background);     // pages (leaves the spine on the left)
      t.fillRect(cx - 4, cy - 6, 11, 2, color);            // two lines of text
      t.fillRect(cx - 4, cy, 11, 2, color);
      break;

    case Icon::Home:
      t.fillTriangle(cx - 14, cy - 1, cx + 14, cy - 1, cx, cy - 14, color);   // roof
      t.fillRect(cx - 10, cy - 1, 20, 14, color);                             // walls
      t.fillRect(cx - 3, cy + 4, 6, 9, background);                           // door
      break;

    case Icon::Check:
      thickLine(cx - 12, cy, cx - 4, cy + 9, color);
      thickLine(cx - 4, cy + 9, cx + 12, cy - 9, color);
      break;

    case Icon::Cross:
      thickLine(cx - 10, cy - 10, cx + 10, cy + 10, color);
      thickLine(cx - 10, cy + 10, cx + 10, cy - 10, color);
      break;
  }
}

}  // namespace Icons
