// SIMULATOR ONLY. Time and pins. The real button, buzzer and backlight drivers run on top of these.
#include <Arduino.h>
#include <emscripten.h>
#include "config/pins.h"
#include "sim.h"

// The buzzer is active low: LOW = beep.
EM_JS(void, simBuzzer, (int on), { if (Module.onBuzzer) Module.onBuzzer(!!on); });

namespace {

double firstHostMs = -1;
uint32_t elapsedMs = 0;
uint32_t skippedMs = 0;
bool buttonDown = false;
int backlightLevel = 0;

}  // namespace

// Starts at 1000 so that "long ago" (0) is already further back than any debounce time.
uint32_t millis() { return 1000 + elapsedMs + skippedMs; }

void pinMode(uint8_t, uint8_t) {}

void digitalWrite(uint8_t pin, uint8_t value) {
  if (pin == Pins::kBuzzer) simBuzzer(value == LOW);
}

int digitalRead(uint8_t pin) { return (pin == Pins::kEncSw && buttonDown) ? LOW : HIGH; }

void analogWrite(uint8_t pin, int value) {
  if (pin == Pins::kLcdBacklight) backlightLevel = value;
}

void analogWriteFreq(uint32_t) {}

namespace Sim {

void setHostMs(double hostMs) {
  if (firstHostMs < 0) firstHostMs = hostMs;
  elapsedMs = (uint32_t)(hostMs - firstHostMs);
}

void skipMs(uint32_t ms) { skippedMs += ms; }
void setButton(bool down) { buttonDown = down; }
int backlight() { return backlightLevel; }

}  // namespace Sim
