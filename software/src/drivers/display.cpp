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
  return fonts.getUTF8Width(utf8);
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
  fonts.drawUTF8(x, baselineY, utf8);
}

}  // namespace Display
