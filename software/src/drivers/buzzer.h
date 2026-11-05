// Active buzzer module (low-level trigger). Non-blocking: play() starts a pattern,
// update() must be called from loop() to advance it.
#pragma once
#include <Arduino.h>

namespace Buzzer {

enum class Pattern : uint8_t {
  Click,   // 15 ms tick for UI feedback
  Short,   // 60 ms
  Long,    // 300 ms
  Nudge,   // three soft beeps: "target time reached"
  Alert,   // three longer beeps: "something happened that needs a look" (power was cut)
};

// Call this FIRST in setup(): it drives the pin HIGH (silent) before anything else can glitch it.
void begin();

// Starts a pattern. A new pattern replaces the one that is currently playing.
void play(Pattern pattern);

void stop();
void update();

}  // namespace Buzzer
