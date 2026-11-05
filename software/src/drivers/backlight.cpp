#include "drivers/backlight.h"
#include "config/config.h"
#include "config/pins.h"

namespace {

uint8_t level = Config::kMaxBrightness;

void apply() { analogWrite(Pins::kLcdBacklight, level); }

}  // namespace

namespace Backlight {

void begin() {
  pinMode(Pins::kLcdBacklight, OUTPUT);
  analogWriteFreq(Config::kBacklightPwmHz);   // must be set before analogWrite()
  apply();
}

void set(int value) {
  level = constrain(value, Config::kMinBrightness, Config::kMaxBrightness);
  apply();
}

uint8_t get() { return level; }

void adjust(int steps) { set((int)level + steps * Config::kBrightnessStep); }

}  // namespace Backlight
