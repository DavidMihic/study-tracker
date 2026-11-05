// EC11 rotary encoder, rotation only (the push switch is in button.h).
// Uses a full-step state table: it reports one click only after a complete A/B cycle,
// so contact bounce and half-finished clicks are ignored.
#pragma once
#include <Arduino.h>

namespace Encoder {

void begin();

// Call as often as possible. Returns +1 for one click clockwise, -1 for one click
// counter-clockwise, 0 when nothing happened.
int8_t poll();

}  // namespace Encoder
