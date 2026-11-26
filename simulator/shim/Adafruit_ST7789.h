// SIMULATOR ONLY. Replaces the ST7789 driver: the real Adafruit_GFX drawing code writes into a
// 320 x 240 RGB565 frame buffer, which the web page copies onto a canvas.
#pragma once
#include <Adafruit_GFX.h>
#include <SPI.h>

class Adafruit_ST7789 : public Adafruit_GFX {
 public:
  static constexpr int16_t kWidth = 320;
  static constexpr int16_t kHeight = 240;

  Adafruit_ST7789(SPIClass*, int8_t, int8_t, int8_t) : Adafruit_GFX(240, 320) {}

  void init(uint16_t, uint16_t) {}
  void setSPISpeed(uint32_t) {}

  // Coordinates arrive already in the rotated (landscape) system, so they are stored as they come.
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
    frame[y * kWidth + x] = color;
    dirty = true;
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    const int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    const int x1 = x + w > kWidth ? kWidth : x + w, y1 = y + h > kHeight ? kHeight : y + h;
    for (int row = y0; row < y1; row++) {
      for (int column = x0; column < x1; column++) frame[row * kWidth + column] = color;
    }
    dirty = true;
  }

  void fillScreen(uint16_t color) override { fillRect(0, 0, kWidth, kHeight, color); }

  uint16_t frame[kWidth * kHeight] = {};
  bool dirty = true;
};
