#include "ui/icons.h"
#include <math.h>
#include "drivers/display.h"

namespace {

constexpr float kPi = 3.14159265f;

// A line drawn 3 pixels thick.
void thickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  Adafruit_ST7789& t = Display::tft();
  t.drawLine(x0, y0, x1, y1, color);
  t.drawLine(x0 + 1, y0, x1 + 1, y1, color);
  t.drawLine(x0, y0 + 1, x1, y1 + 1, color);
}

// A rectangle that starts at distance r0 from the centre and ends at r1, pointing in direction `angle`
// (radians), halfWidth wide on each side. Drawn as two triangles.
void spoke(int16_t cx, int16_t cy, float angle, float r0, float r1, float halfWidth, uint16_t color) {
  float ux = cosf(angle), uy = sinf(angle);   // along the spoke
  float px = -uy, py = ux;                    // across it
  int16_t ax = cx + (int16_t)lroundf(ux * r0 + px * halfWidth), ay = cy + (int16_t)lroundf(uy * r0 + py * halfWidth);
  int16_t bx = cx + (int16_t)lroundf(ux * r0 - px * halfWidth), by = cy + (int16_t)lroundf(uy * r0 - py * halfWidth);
  int16_t dx = cx + (int16_t)lroundf(ux * r1 + px * halfWidth), dy = cy + (int16_t)lroundf(uy * r1 + py * halfWidth);
  int16_t ex = cx + (int16_t)lroundf(ux * r1 - px * halfWidth), ey = cy + (int16_t)lroundf(uy * r1 - py * halfWidth);
  Adafruit_ST7789& t = Display::tft();
  t.fillTriangle(ax, ay, bx, by, dx, dy, color);
  t.fillTriangle(bx, by, ex, ey, dx, dy, color);
}

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
      // Eight wide, short teeth on a round body, with a hole: reads as a cog, not a flower.
      for (int k = 0; k < 8; k++) spoke(cx, cy, k * kPi / 4, 8, 14, 3.5f, color);
      t.fillCircle(cx, cy, 10, color);
      t.fillCircle(cx, cy, 4, background);
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

    case Icon::Snowflake:
      // Three lines through the centre = six arms, each with a little cross-stroke near its tip.
      for (int k = 0; k < 6; k++) {
        float a = k * kPi / 3;
        spoke(cx, cy, a, 0, 14, 1.0f, color);
        spoke(cx + (int16_t)lroundf(cosf(a) * 8), cy + (int16_t)lroundf(sinf(a) * 8), a + kPi / 2, 0, 4, 0.8f, color);
        spoke(cx + (int16_t)lroundf(cosf(a) * 8), cy + (int16_t)lroundf(sinf(a) * 8), a - kPi / 2, 0, 4, 0.8f, color);
      }
      t.fillCircle(cx, cy, 2, color);
      break;

    case Icon::Sun:
      t.fillCircle(cx, cy, 7, color);
      for (int k = 0; k < 8; k++) spoke(cx, cy, k * kPi / 4, 10, 15, 1.5f, color);
      break;

    case Icon::Star: {
      // Five points: outer radius 8, inner 3.5, filled as a fan of triangles from the centre.
      int16_t px[10], py[10];
      for (int k = 0; k < 10; k++) {
        float r = (k % 2 == 0) ? 8.0f : 3.5f;
        float a = -kPi / 2 + k * kPi / 5;
        px[k] = cx + (int16_t)lroundf(cosf(a) * r);
        py[k] = cy + (int16_t)lroundf(sinf(a) * r);
      }
      for (int k = 0; k < 10; k++) t.fillTriangle(cx, cy, px[k], py[k], px[(k + 1) % 10], py[(k + 1) % 10], color);
      break;
    }

    case Icon::Back:
      t.fillTriangle(cx - 12, cy, cx + 8, cy - 13, cx + 8, cy + 13, color);
      break;

    case Icon::BackSmall:
      t.fillTriangle(cx - 5, cy, cx + 4, cy - 6, cx + 4, cy + 6, color);
      break;

    case Icon::Return:
      t.fillRect(cx - 7, cy - 7, 2, 11, color);                          // down ...
      t.fillRect(cx - 7, cy + 2, 11, 2, color);                          // ... then right
      t.fillTriangle(cx + 8, cy + 3, cx + 2, cy - 2, cx + 2, cy + 8, color);   // arrow head
      break;
  }
}

}  // namespace Icons
