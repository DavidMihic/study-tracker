// Two small grey arrow heads (no stem) in the right corners of a scrollable menu: one at the top when the
// list can scroll up, one at the bottom when it can scroll down. They redraw only when that changes.
// Menus keep their title and hint fields narrower than the screen (see kReservedWidth) so these corners
// stay free.
#pragma once
#include <stdint.h>

class ScrollArrows {
 public:
  static constexpr int16_t kReservedWidth = 28;   // pixels at the right edge that the fields leave free

  // Call right after the screen was cleared (nothing is drawn yet).
  void reset() { up_ = down_ = false; }

  void update(bool canScrollUp, bool canScrollDown);

 private:
  bool up_ = false, down_ = false;
};
