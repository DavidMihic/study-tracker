#include "drivers/text_field.h"
#include <string.h>

namespace {

// Shortens a UTF-8 string until it fits into maxWidth pixels, ending with "...".
// Removes whole characters (a Croatian letter is two bytes, so never cut in the middle of one).
void fitToWidth(Display::Font font, char* text, size_t capacity, int16_t maxWidth) {
  if (Display::textWidth(font, text) <= maxWidth) return;

  size_t length = strlen(text);
  char candidate[96];
  while (length > 0) {
    do { length--; } while (length > 0 && (text[length] & 0xC0) == 0x80);   // step back one character
    if (length + 4 > sizeof candidate) continue;
    memcpy(candidate, text, length);
    memcpy(candidate + length, "...", 4);   // includes the terminating zero
    if (Display::textWidth(font, candidate) <= maxWidth) {
      strncpy(text, candidate, capacity - 1);
      text[capacity - 1] = '\0';
      return;
    }
  }
  text[0] = '\0';
}

}  // namespace

void TextField::configure(int16_t x, int16_t y, int16_t width, int16_t height, Display::Font font,
                          Align align, uint16_t foreground, uint16_t background, int16_t padding) {
  x_ = x; y_ = y; width_ = width; height_ = height; padding_ = padding;
  font_ = font;
  align_ = align;
  foreground_ = foreground;
  background_ = background;
  drawn_ = false;
}

void TextField::set(const char* utf8) { set(utf8, foreground_, background_); }

void TextField::set(const char* utf8, uint16_t foreground, uint16_t background) {
  bool unchanged = drawn_ && foreground == foreground_ && background == background_ &&
                   strncmp(text_, utf8, kMaxBytes - 1) == 0;
  if (unchanged) return;
  foreground_ = foreground;
  background_ = background;
  strncpy(text_, utf8, kMaxBytes - 1);
  text_[kMaxBytes - 1] = '\0';
  draw();
}

void TextField::setColors(uint16_t foreground, uint16_t background) {
  if (foreground == foreground_ && background == background_) return;
  foreground_ = foreground;
  background_ = background;
  if (drawn_) draw();
}

void TextField::draw() {
  Display::tft().fillRect(x_, y_, width_, height_, background_);

  char shown[kMaxBytes];
  strncpy(shown, text_, kMaxBytes - 1);
  shown[kMaxBytes - 1] = '\0';
  int16_t room = width_ - 2 * padding_;
  fitToWidth(font_, shown, sizeof shown, room);

  int16_t textWidth = Display::textWidth(font_, shown);
  int16_t left = x_ + padding_;
  if (align_ == Align::Center) left = x_ + (width_ - textWidth) / 2;
  if (align_ == Align::Right)  left = x_ + width_ - padding_ - textWidth;

  // Centre the text vertically: the glyphs span from ascent above to descent below the baseline.
  int16_t ascent = Display::fontAscent(font_);
  int16_t descent = Display::fontDescent(font_);    // negative
  int16_t baseline = y_ + (height_ - (ascent - descent)) / 2 + ascent;

  Display::drawText(font_, left, baseline, shown, foreground_);
  drawn_ = true;
}
