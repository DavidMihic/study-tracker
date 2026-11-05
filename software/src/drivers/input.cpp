#include "drivers/input.h"
#include "drivers/button.h"
#include "drivers/encoder.h"

namespace {

constexpr uint8_t kQueueSize = 16;   // plenty: the main loop drains it every pass

Input::Event queue[kQueueSize];
uint8_t head = 0;    // next to read
uint8_t count = 0;
uint32_t lastActivity = 0;

void push(Input::Event::Type type, int8_t delta) {
  lastActivity = millis();
  if (count == kQueueSize) return;   // full: drop the newest event
  queue[(head + count) % kQueueSize] = {type, delta};
  count++;
}

}  // namespace

namespace Input {

void begin() {
  Encoder::begin();
  Button::begin();
  lastActivity = millis();
}

void poll() {
  int8_t turn = Encoder::poll();
  if (turn != 0) push(Event::Type::Turn, turn);

  switch (Button::poll()) {
    case Button::Event::Click:     push(Event::Type::Click, 0); break;
    case Button::Event::LongPress: push(Event::Type::LongPress, 0); break;
    case Button::Event::None:      break;
  }
}

bool next(Event& out) {
  if (count == 0) return false;
  out = queue[head];
  head = (head + 1) % kQueueSize;
  count--;
  return true;
}

uint32_t lastActivityMs() { return lastActivity; }

}  // namespace Input
