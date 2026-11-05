// Encoder push switch, debounced. Distinguishes a click (released before the long-press
// time) from a long press (fires once while still held, no click on release).
#pragma once
#include <Arduino.h>

namespace Button {

enum class Event : uint8_t { None, Click, LongPress };

void begin();

// Call as often as possible. Returns the event that just happened, or Event::None.
Event poll();

}  // namespace Button
