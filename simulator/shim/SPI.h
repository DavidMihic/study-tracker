// SIMULATOR ONLY. There is no SPI bus; display.cpp only picks its pins.
#pragma once
#include <stdint.h>

class SPIClass {
 public:
  void setSCK(uint8_t) {}
  void setTX(uint8_t) {}
};

inline SPIClass SPI;
