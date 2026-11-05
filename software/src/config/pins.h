// GPIO numbers (not the silkscreen labels). This is the pin map from CLAUDE.md section 4.
// If the schematic changes, change it here and nowhere else.
#pragma once
#include <Arduino.h>

namespace Pins {

// ST7789 display (SPI0)
constexpr uint8_t kLcdSck       = 18;  // silk "SCK"
constexpr uint8_t kLcdMosi      = 19;  // silk "MO"
constexpr uint8_t kLcdCs        = 10;  // silk "10"
constexpr uint8_t kLcdDc        = 11;  // silk "11"
constexpr uint8_t kLcdRst       = 12;  // silk "12"
constexpr uint8_t kLcdBacklight = 9;   // silk "9"

// EC11 rotary encoder
constexpr uint8_t kEncA  = 7;          // silk "5"
constexpr uint8_t kEncB  = 8;          // silk "6"
constexpr uint8_t kEncSw = 24;         // silk "24"

// Active buzzer module, low-level trigger: HIGH = silent, LOW = beep
constexpr uint8_t kBuzzer = 25;        // silk "25"

// DS3231 on I2C (the core's Wire object works with these pins)
constexpr uint8_t kRtcSda = 2;         // silk "SDA"
constexpr uint8_t kRtcScl = 3;         // silk "SCL"

}  // namespace Pins
