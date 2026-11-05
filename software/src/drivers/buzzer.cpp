#include "drivers/buzzer.h"
#include "config/pins.h"

namespace {

// One step = beep for onMs, then stay silent for offMs (0 = nothing after the last beep).
struct Step { uint16_t onMs; uint16_t offMs; };

const Step kClick[] = {{15, 0}};
const Step kShort[] = {{60, 0}};
const Step kLong[]  = {{300, 0}};
const Step kNudge[] = {{80, 120}, {80, 120}, {80, 0}};

template <size_t N>
constexpr uint8_t countOf(const Step (&)[N]) { return N; }

const Step* steps = nullptr;   // pattern being played, nullptr = idle
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
bool soundOn = false;
uint32_t phaseEndsAt = 0;

// The module triggers on LOW, so "sound on" means pin LOW.
void setSound(bool on) {
  digitalWrite(Pins::kBuzzer, on ? LOW : HIGH);
  soundOn = on;
}

}  // namespace

namespace Buzzer {

void begin() {
  pinMode(Pins::kBuzzer, OUTPUT);
  setSound(false);
}

void play(Pattern pattern) {
  switch (pattern) {
    case Pattern::Click: steps = kClick; stepCount = countOf(kClick); break;
    case Pattern::Short: steps = kShort; stepCount = countOf(kShort); break;
    case Pattern::Long:  steps = kLong;  stepCount = countOf(kLong);  break;
    case Pattern::Nudge: steps = kNudge; stepCount = countOf(kNudge); break;
  }
  stepIndex = 0;
  setSound(true);
  phaseEndsAt = millis() + steps[0].onMs;
}

void stop() {
  steps = nullptr;
  setSound(false);
}

void update() {
  if (!steps || (int32_t)(millis() - phaseEndsAt) < 0) return;

  if (soundOn) {
    // Beep finished: go silent, then wait out the gap (if any).
    setSound(false);
    if (steps[stepIndex].offMs == 0) { steps = nullptr; return; }
    phaseEndsAt = millis() + steps[stepIndex].offMs;
  } else {
    // Gap finished: start the next beep, or finish the pattern.
    if (++stepIndex >= stepCount) { steps = nullptr; return; }
    setSound(true);
    phaseEndsAt = millis() + steps[stepIndex].onMs;
  }
}

}  // namespace Buzzer
