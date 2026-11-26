// SIMULATOR ONLY. Base class of Adafruit_GFX and the u8g2 text library.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

class __FlashStringHelper;
#define F(text) (reinterpret_cast<const __FlashStringHelper*>(text))

// Only what Adafruit_GFX::getTextBounds() needs to compile.
class String : public std::string {
 public:
  using std::string::string;
};

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) { return 1; }
  virtual size_t write(const uint8_t* bytes, size_t length) {
    size_t n = 0;
    while (n < length && write(bytes[n])) n++;
    return n;
  }
  size_t print(const char* text) { return write((const uint8_t*)text, __builtin_strlen(text)); }
};
