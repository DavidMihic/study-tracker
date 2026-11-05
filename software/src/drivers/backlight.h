// Display backlight: PWM on the BL pin. The module draws only ~1 mA there, so the GPIO drives it directly.
#pragma once
#include <Arduino.h>

namespace Backlight {

void begin();

// Sets brightness; values are clamped to Config::kMinBrightness..kMaxBrightness
// so the screen can never go completely dark.
void set(int level);
uint8_t get();

// Relative change (for the encoder): adjust(+1) = one brightness step brighter.
void adjust(int steps);

}  // namespace Backlight
