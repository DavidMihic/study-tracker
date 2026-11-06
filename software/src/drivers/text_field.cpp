#include "drivers/text_field.h"
#include <string.h>

namespace {

// Removes the last UTF-8 character of text (a Croatian letter is two bytes, so never cut in the middle).
// Returns the new length.
size_t dropLastChar(const char* text, size_t length) {
  if (length == 0) return 0;
  do { length--; } while (length > 0 && (text[length] & 0xC0) == 0x80);
  return length;
}

// Shortens a UTF-8 string until it fits into maxWidth pixels, ending with "...".
void fitWithEllipsis(Display::Font font, char* text, size_t capacity, int16_t maxWidth) {
  if (Display::textWidth(font, text) <= maxWidth) return;

  size_t length = strlen(text);
  char candidate[96];
  while (length > 0) {
    length = dropLastChar(text, length);
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

// Shortens the string until it fits, with no "..." (the scrolling window).
void cutToWidth(Display::Font font, char* text, int16_t maxWidth) {
  size_t length = strlen(text);
  while (length > 0 && Display::textWidth(font, text) > maxWidth) {
    length = dropLastChar(text, length);
    text[length] = '\0';
  }
}

// Pointer to the text after skipping `chars` UTF-8 characters.
const char* skipChars(const char* text, int chars) {
  while (*text && chars > 0) {
    text++;
    while ((*text & 0xC0) == 0x80) text++;
    chars--;
  }
  return text;
}

}  // namespace

void TextField::configure(int16_t x, int16_t y, int16_t width, int16_t height, Display::Font font,
                          Align align, uint16_t foreground, uint16_t background, int16_t padding) {
  x_ = x; y_ = y; width_ = width; height_ = height; padding_ = padding;
  font_ = font;
  align_ = align;
  foreground_ = foreground;
  background_ = background;
  iconSide_ = IconSide::None;
  scroll_ = -1;
  stale_ = false;
  drawn_ = false;
}

int16_t TextField::room() const {
  return width_ - 2 * padding_ - (iconSide_ != IconSide::None ? kIconSlot : 0);
}

void TextField::set(const char* utf8) { set(utf8, foreground_, background_); }

void TextField::set(const char* utf8, uint16_t foreground, uint16_t background) {
  bool unchanged = drawn_ && !stale_ && foreground == foreground_ && background == background_ &&
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

void TextField::setIcon(Icons::Icon icon, uint16_t color, IconSide side) {
  if (iconSide_ == side && icon_ == icon && iconColor_ == color) return;
  icon_ = icon;
  iconColor_ = color;
  iconSide_ = side;
  stale_ = true;
}

void TextField::setScroll(int16_t skip) {
  if (skip == scroll_) return;
  scroll_ = skip;
  stale_ = true;
}

bool TextField::overflows() const { return Display::textWidth(font_, text_) > room(); }

int16_t TextField::scrollSteps() const {
  int16_t steps = 0;
  const char* rest = text_;
  while (*rest && Display::textWidth(font_, rest) > room()) {
    rest = skipChars(rest, 1);
    steps++;
  }
  return steps;
}

// The big clock font is proportional, so a changing digit would shift the whole string sideways.
// Instead every character gets a cell of fixed width (all digits as wide as the widest digit), and a
// second tick redraws only the one or two cells whose character changed.
void TextField::drawCells() {
  static int16_t digitWidth = 0, colonWidth = 0;
  if (digitWidth == 0) {
    for (char d = '0'; d <= '9'; d++) {
      char s[2] = {d, '\0'};
      int16_t w = Display::textWidth(font_, s);
      if (w > digitWidth) digitWidth = w;
    }
    colonWidth = Display::textWidth(font_, ":") + 4;
  }
  auto cellWidth = [](char c) { return c == ':' ? colonWidth : digitWidth; };

  const size_t length = strlen(text_);
  int16_t total = 0;
  for (size_t i = 0; i < length; i++) total += cellWidth(text_[i]);
  int16_t left = x_ + padding_;
  if (align_ == Align::Center) left = x_ + (width_ - total) / 2;
  if (align_ == Align::Right)  left = x_ + width_ - padding_ - total;

  // Everything is redrawn when the layout or the colours changed (or the screen was cleared).
  const bool full = !drawn_ || stale_ || foreground_ != cellForeground_ || background_ != cellBackground_ ||
                    left != cellLeft_ || length != strlen(cellText_);
  if (full) Display::tft().fillRect(x_, y_, width_, height_, background_);

  const int16_t ascent = Display::fontAscent(font_);
  const int16_t descent = Display::fontDescent(font_);
  const int16_t baseline = y_ + (height_ - (ascent - descent)) / 2 + ascent;

  int16_t cellX = left;
  for (size_t i = 0; i < length; i++) {
    const int16_t w = cellWidth(text_[i]);
    if (full || text_[i] != cellText_[i]) {
      if (!full) Display::tft().fillRect(cellX, y_, w, height_, background_);
      if (text_[i] != ' ') {
        char glyph[2] = {text_[i], '\0'};
        Display::drawText(font_, cellX + (w - Display::textWidth(font_, glyph)) / 2, baseline, glyph, foreground_);
      }
    }
    cellX += w;
  }

  strncpy(cellText_, text_, kMaxBytes - 1);
  cellText_[kMaxBytes - 1] = '\0';
  cellForeground_ = foreground_;
  cellBackground_ = background_;
  cellLeft_ = left;
  drawn_ = true;
  stale_ = false;
}

void TextField::draw() {
  if (font_ == Display::Font::Clock) { drawCells(); return; }
  Display::tft().fillRect(x_, y_, width_, height_, background_);

  char shown[kMaxBytes];
  strncpy(shown, scroll_ >= 0 ? skipChars(text_, scroll_) : text_, kMaxBytes - 1);
  shown[kMaxBytes - 1] = '\0';
  if (scroll_ >= 0) cutToWidth(font_, shown, room());
  else fitWithEllipsis(font_, shown, sizeof shown, room());

  const int16_t slot = (iconSide_ != IconSide::None) ? kIconSlot : 0;
  int16_t textWidth = Display::textWidth(font_, shown);
  int16_t total = textWidth + slot;
  int16_t left = x_ + padding_;
  if (align_ == Align::Center) left = x_ + (width_ - total) / 2;
  if (align_ == Align::Right)  left = x_ + width_ - padding_ - total;
  int16_t textX = left + (iconSide_ == IconSide::Before ? slot : 0);

  // Centre the text vertically: the glyphs span from ascent above to descent below the baseline.
  int16_t ascent = Display::fontAscent(font_);
  int16_t descent = Display::fontDescent(font_);    // negative
  int16_t baseline = y_ + (height_ - (ascent - descent)) / 2 + ascent;
  Display::drawText(font_, textX, baseline, shown, foreground_);

  if (iconSide_ != IconSide::None) {
    int16_t iconX = (iconSide_ == IconSide::Before) ? left + slot / 2 : textX + textWidth + slot / 2 + 2;
    Icons::draw(icon_, iconX, y_ + height_ / 2, iconColor_, background_);
  }
  drawn_ = true;
  stale_ = false;
}
