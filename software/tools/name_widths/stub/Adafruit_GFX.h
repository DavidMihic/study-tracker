// Just enough of Adafruit_GFX / Arduino's Print for the u8g2 library to compile on the PC.
// Only text WIDTH is measured, so nothing is ever drawn.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) { return 1; }
  virtual size_t write(const uint8_t*, size_t n) { return n; }
};

class Adafruit_GFX {
 public:
  virtual ~Adafruit_GFX() {}
  virtual void drawPixel(int16_t, int16_t, uint16_t) {}
  virtual void fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  virtual void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) {}
  virtual void drawFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
};
