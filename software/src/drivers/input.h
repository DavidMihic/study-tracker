// Single input channel for the UI: turns, clicks and long presses arrive as events in a queue.
// Screens only ever look at this; they never talk to the encoder or button drivers directly.
#pragma once
#include <Arduino.h>

namespace Input {

struct Event {
  enum class Type : uint8_t { Turn, Click, LongPress };
  Type type;
  int8_t delta;   // Turn only: +1 clockwise, -1 counter-clockwise
};

void begin();

// Reads the encoder and the button and queues events. Call every loop().
void poll();

// Takes the oldest queued event. Returns false when the queue is empty.
bool next(Event& out);

// millis() of the last event, for the idle timeout (screensaver).
uint32_t lastActivityMs();

}  // namespace Input
