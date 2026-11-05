// ST7789 display. Owns the SPI setup, the Adafruit driver object and the text fonts.
// Rule for the whole UI: never redraw the whole screen. Draw static parts once and update only the
// fields that changed (see TextField in text_field.h, which is what screens normally use).
#pragma once
#include <Arduino.h>
#include <Adafruit_ST7789.h>

namespace Display {

// The fonts the UI uses. All except Clock contain the Croatian letters (č ć š đ ž and the capitals);
// this was checked in the font data. Clock contains digits, ':' and space only.
enum class Font : uint8_t {
  Small,     // 12 px, hints and secondary text
  Body,      // 14 px bold, list rows
  Heading,   // 18 px bold, subject names and titles
  Clock,     // 42 px digits, the big timers and the clock
};

void begin();

// Direct access to the Adafruit_GFX drawing functions (fillRect, drawLine, ...).
Adafruit_ST7789& tft();

// Clears the whole screen. Takes ~120 ms and shows as a sweep, so use it only when switching screens.
void clear(uint16_t color);

// Text measuring and drawing (UTF-8). y is the BASELINE of the text.
int16_t textWidth(Font font, const char* utf8);
int16_t fontAscent(Font font);    // pixels above the baseline (positive)
int16_t fontDescent(Font font);   // pixels below the baseline (negative)
void drawText(Font font, int16_t x, int16_t baselineY, const char* utf8, uint16_t color);

}  // namespace Display
