// A rectangle on the screen that shows one line of text. set() redraws the rectangle only when the
// text (or the colours, icon or scroll position) changed, so calling it every loop is cheap. This is how
// the UI follows the rule "redraw only what changed".
#pragma once
#include <Arduino.h>
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/icons.h"

class TextField {
 public:
  enum class Align : uint8_t { Left, Center, Right };
  enum class IconSide : uint8_t { None, Before, After };   // a small icon in front of / right after the text

  // Where the field is and how it looks. padding = space between the field edge and the text.
  void configure(int16_t x, int16_t y, int16_t width, int16_t height, Display::Font font, Align align,
                 uint16_t foreground, uint16_t background = Colors::kBackground, int16_t padding = 0);

  // Shows the text. Does nothing if nothing changed since the last draw. Text that is too wide is
  // shortened with "..." (or, in scroll mode, cut at the edge).
  void set(const char* utf8);

  // Same, and sets the colours in the same step, so a changed row is drawn once (no flicker).
  void set(const char* utf8, uint16_t foreground, uint16_t background);

  // Changes the colours (redraws right away if something is already shown).
  void setColors(uint16_t foreground, uint16_t background);

  // Icon next to the text. These only remember the values: the next set() draws them (so a row is
  // drawn once). Call set() after changing them.
  void setIcon(Icons::Icon icon, uint16_t color, IconSide side);
  void clearIcon() { if (iconSide_ != IconSide::None) { iconSide_ = IconSide::None; stale_ = true; } }

  // Scroll mode, for a focused row with a long name: skipChars = how many characters at the start are
  // left out, so the end of the text slides into view. -1 = off (too-long text ends with "...").
  // Needs a set() afterwards, like setIcon().
  void setScroll(int16_t skipChars);
  bool overflows() const;          // the whole current text is wider than the room
  int16_t scrollSteps() const;     // how many characters must be skipped for the end to be visible

  // Moves/resizes the field horizontally (a row whose right column is narrower gives the space to the
  // left one). Needs a set() afterwards, like setIcon().
  void setBounds(int16_t x, int16_t width) {
    if (x == x_ && width == width_) return;
    x_ = x; width_ = width; stale_ = true;
  }

  // Call after the screen was cleared: the next set() draws again even if the text is the same.
  void invalidate() { drawn_ = false; }

 private:
  static constexpr size_t kMaxBytes = 96;
  static constexpr int16_t kIconSlot = 22;   // space reserved for an icon

  void draw();
  int16_t room() const;

  int16_t x_ = 0, y_ = 0, width_ = 0, height_ = 0, padding_ = 0;
  Display::Font font_ = Display::Font::Small;
  Align align_ = Align::Left;
  uint16_t foreground_ = Colors::kText;
  uint16_t background_ = Colors::kBackground;
  char text_[kMaxBytes] = "";
  bool drawn_ = false;
  bool stale_ = false;     // icon or scroll changed since the last draw
  Icons::Icon icon_ = Icons::Icon::Star;
  uint16_t iconColor_ = Colors::kText;
  IconSide iconSide_ = IconSide::None;
  int16_t scroll_ = -1;
};
