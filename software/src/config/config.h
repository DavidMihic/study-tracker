// Tunable values in one place. Everything here was chosen from bench measurements
// (see CLAUDE.md); change a number here, not in the driver code.
#pragma once
#include <Arduino.h>

namespace Config {

// Input
constexpr uint32_t kButtonDebounceMs = 25;
constexpr uint32_t kLongPressMs      = 800;

// Display
constexpr uint32_t kSpiHz            = 32000000UL;  // 62.5 MHz is no faster (library is CPU-bound)
constexpr uint8_t  kRotation         = 1;           // landscape, 320 x 240
constexpr int16_t  kScreenWidth      = 320;
constexpr int16_t  kScreenHeight     = 240;

// Backlight (PWM on the BL pin; 20 kHz is above hearing range)
constexpr uint32_t kBacklightPwmHz   = 20000;
constexpr uint8_t  kMinBrightness    = 8;           // dimmest setting that is still readable
constexpr uint8_t  kMaxBrightness    = 255;
constexpr uint8_t  kBrightnessStep   = 16;

// Pop-up card menus close by themselves (changing nothing) after this long without input.
constexpr uint32_t kMenuTimeoutMs    = 15000;

// RTC
constexpr uint32_t kRtcPollMs        = 100;         // read the DS3231 this often; the screen redraws only when the second changes

// Screensaver (build step 5). Study rules live in config/study_rules.h.
constexpr uint32_t kIdleToScreensaverMs = 120000UL;
constexpr uint8_t  kScreensaverBrightness = 24;   // the backlight never goes above this while it runs

}  // namespace Config
