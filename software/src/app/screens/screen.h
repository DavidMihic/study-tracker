// Common interface for every screen. The app calls these; a screen never talks to hardware pins.
#pragma once
#include "drivers/input.h"

class Screen {
 public:
  virtual ~Screen() = default;

  // The screen was just switched to: clear the display and draw everything that does not change.
  virtual void enter() = 0;

  // One input event (turn, click, long press).
  virtual void onEvent(const Input::Event& event) = 0;

  // Called every loop(). Redraw only the fields that changed since last time.
  virtual void update() = 0;
};
