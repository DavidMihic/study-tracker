// Screensaver: a green heartbeat line sweeping across a dimmed screen with the clock above it.
// Started by the Home screen after Config::kIdleToScreensaverMs without input. The first turn or press
// only wakes the device: it is used up for that and does nothing else (she can not trigger something by
// waking it).
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"

class ScreensaverScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  void drawClock();
  void drawTrace();   // advances the heartbeat line to where it should be now

  TextField clock_, date_;
  uint32_t startMs_ = 0;
  uint32_t lastPixel_ = 0;   // how many pixels of the trace have been drawn since enter()
  int16_t lastY_ = 0;
  bool redrawClock_ = true;
};
