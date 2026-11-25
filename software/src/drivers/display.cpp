#include "drivers/display.h"
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config/config.h"
#include "config/pins.h"
#include "ui/colors.h"

namespace {

Adafruit_ST7789 tftDriver(&SPI, Pins::kLcdCs, Pins::kLcdDc, Pins::kLcdRst);
U8G2_FOR_ADAFRUIT_GFX fonts;   // draws UTF-8 text with u8g2 fonts onto the Adafruit driver

// Font choices. The "_te" fonts contain Latin Extended-A (č ć š đ ž); "_tn" is digits only.
const uint8_t* fontData(Display::Font font) {
  switch (font) {
    case Display::Font::Small:   return u8g2_font_helvR12_te;
    case Display::Font::Body:    return u8g2_font_helvB14_te;
    case Display::Font::Heading: return u8g2_font_helvB18_te;
    case Display::Font::Clock:   return u8g2_font_logisoso42_tn;
  }
  return u8g2_font_helvR12_te;
}

// The fonts squeeze Š and Ž to the height of a plain capital, which leaves a letter body no taller than a
// lowercase one ("Što" looks like "što"). So these two are drawn as a full-size S or Z with a separate
// caron on top, copied from the font's own Č (which is drawn properly).
struct Caron {
  uint8_t width, height;
  uint8_t rise;        // pixels from the baseline up to the caron's top row
  uint16_t rows[4];    // one bit per pixel, the highest used bit is the left edge
};

const Caron& caronFor(Display::Font font) {
  static const Caron kSmall   = {5, 3, 16, {0b10001, 0b01010, 0b00100}};
  static const Caron kBody    = {7, 3, 18, {0b1110111, 0b0111110, 0b0011100}};
  static const Caron kHeading = {9, 4, 24, {0b111000111, 0b011101110, 0b001111100, 0b000111000}};
  switch (font) {
    case Display::Font::Body:    return kBody;
    case Display::Font::Heading: return kHeading;
    default:                     return kSmall;   // Clock has only digits, so it never gets here
  }
}

// If text starts with Š or Ž (two bytes in UTF-8), returns the plain letter under the caron, otherwise 0.
char capitalUnderCaron(const char* text) {
  if ((uint8_t)text[0] != 0xC5) return 0;
  if ((uint8_t)text[1] == 0xA0) return 'S';
  if ((uint8_t)text[1] == 0xBD) return 'Z';
  return 0;
}

// Copies text up to the next Š / Ž (or the end, or a full buffer) into out. Returns the rest of the text.
const char* copyRun(const char* text, char* out, size_t capacity) {
  size_t n = 0;
  while (*text && !capitalUnderCaron(text)) {
    // Stop before a character that might not fit, so a UTF-8 letter is never cut in half.
    if ((*text & 0xC0) != 0x80 && n + 4 >= capacity) break;
    out[n++] = *text++;
  }
  out[n] = '\0';
  return text;
}

void drawCaron(const Caron& caron, int16_t x, int16_t baselineY, uint16_t color) {
  for (uint8_t row = 0; row < caron.height; row++) {
    for (uint8_t column = 0; column < caron.width; column++) {
      if (caron.rows[row] & (1u << (caron.width - 1 - column))) {
        tftDriver.drawPixel(x + column, baselineY - caron.rise + row, color);
      }
    }
  }
}

}  // namespace

namespace Display {

void begin() {
  SPI.setSCK(Pins::kLcdSck);
  SPI.setTX(Pins::kLcdMosi);
  tftDriver.init(240, 320);                  // sets its own default speed, so change it afterwards
  tftDriver.setSPISpeed(Config::kSpiHz);
  tftDriver.setRotation(Config::kRotation);
  tftDriver.fillScreen(Colors::kBackground);

  fonts.begin(tftDriver);
  fonts.setFontMode(1);                      // transparent: TextField clears its own background
}

Adafruit_ST7789& tft() { return tftDriver; }

void clear(uint16_t color) { tftDriver.fillScreen(color); }

int16_t textWidth(Font font, const char* utf8) {
  fonts.setFont(fontData(font));
  // Measure what drawText() really draws: Š and Ž take the width of S and Z.
  char plain[128];
  size_t n = 0;
  while (*utf8 && n < sizeof plain - 1) {
    const char capital = capitalUnderCaron(utf8);
    plain[n++] = capital ? capital : *utf8;
    utf8 += capital ? 2 : 1;
  }
  plain[n] = '\0';
  return fonts.getUTF8Width(plain);
}

int16_t fontAscent(Font font) {
  fonts.setFont(fontData(font));
  return fonts.getFontAscent();
}

int16_t fontDescent(Font font) {
  fonts.setFont(fontData(font));
  return fonts.getFontDescent();
}

void drawText(Font font, int16_t x, int16_t baselineY, const char* utf8, uint16_t color) {
  fonts.setFont(fontData(font));
  // setFont() switches the library back to "opaque" text (a black box behind every letter) whenever the
  // font changes, so transparent mode must be chosen again after it, or text on a coloured row gets
  // black rectangles.
  fonts.setFontMode(1);
  fonts.setForegroundColor(color);
  while (*utf8) {
    const char capital = capitalUnderCaron(utf8);
    if (capital) {
      const Caron& caron = caronFor(font);
      const int16_t advance = fonts.drawGlyph(x, baselineY, capital);
      drawCaron(caron, x + (advance - caron.width) / 2, baselineY, color);
      x += advance;
      utf8 += 2;
    } else {
      char run[128];
      utf8 = copyRun(utf8, run, sizeof run);
      x += fonts.drawUTF8(x, baselineY, run);
    }
  }
}

}  // namespace Display
