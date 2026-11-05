// A rectangle on the screen that shows one line of text. set() redraws the rectangle only when the
// text (or the colours) changed, so calling it every loop is cheap. This is how the UI follows the
// rule "redraw only what changed".
#pragma once
#include <Arduino.h>
#include "drivers/display.h"
#include "ui/colors.h"

class TextField {
 public:
  enum class Align : uint8_t { Left, Center, Right };

  // Where the field is and how it looks. padding = space between the field edge and the text.
  void configure(int16_t x, int16_t y, int16_t width, int16_t height, Display::Font font, Align align,
                 uint16_t foreground, uint16_t background = Colors::kBackground, int16_t padding = 0);

  // Shows the text. Does nothing if it is already shown. Text that is too wide is shortened with "...".
  void set(const char* utf8);

  // Same, and sets the colours in the same step, so a changed row is drawn once (no flicker).
  void set(const char* utf8, uint16_t foreground, uint16_t background);

  // Changes the colours (redraws right away if something is already shown).
  void setColors(uint16_t foreground, uint16_t background);

  // Call after the screen was cleared: the next set() draws again even if the text is the same.
  void invalidate() { drawn_ = false; }

 private:
  static constexpr size_t kMaxBytes = 96;

  void draw();

  int16_t x_ = 0, y_ = 0, width_ = 0, height_ = 0, padding_ = 0;
  Display::Font font_ = Display::Font::Small;
  Align align_ = Align::Left;
  uint16_t foreground_ = Colors::kText;
  uint16_t background_ = Colors::kBackground;
  char text_[kMaxBytes] = "";
  bool drawn_ = false;
};
