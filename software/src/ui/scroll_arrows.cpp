#include "ui/scroll_arrows.h"
#include "drivers/display.h"
#include "ui/colors.h"

namespace {

constexpr int16_t kCenterX = 308;
constexpr int16_t kTopY = 16;
constexpr int16_t kBottomY = 226;

// An arrow head: two strokes (3 pixels thick) meeting in a point. dir = -1 points up, +1 points down.
void drawHead(int16_t cx, int16_t cy, int dir, uint16_t color) {
  Adafruit_ST7789& t = Display::tft();
  for (int16_t k = -1; k <= 1; k++) {
    t.drawLine(cx - 7 + k, cy - dir * 4, cx + k, cy + dir * 4, color);
    t.drawLine(cx + 7 + k, cy - dir * 4, cx + k, cy + dir * 4, color);
  }
}

void erase(int16_t cy) {
  Display::tft().fillRect(kCenterX - 12, cy - 9, 24, 18, Colors::kBackground);
}

}  // namespace

void ScrollArrows::update(bool canScrollUp, bool canScrollDown) {
  if (canScrollUp != up_) {
    if (canScrollUp) drawHead(kCenterX, kTopY, -1, Colors::kDim);
    else erase(kTopY);
    up_ = canScrollUp;
  }
  if (canScrollDown != down_) {
    if (canScrollDown) drawHead(kCenterX, kBottomY, +1, Colors::kDim);
    else erase(kBottomY);
    down_ = canScrollDown;
  }
}
