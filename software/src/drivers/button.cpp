#include "drivers/button.h"
#include "config/config.h"
#include "config/pins.h"

namespace {

bool isDown = false;          // debounced state
bool longPressFired = false;
uint32_t changedAt = 0;       // last time the debounced state changed
uint32_t pressedAt = 0;

}  // namespace

namespace Button {

void begin() { pinMode(Pins::kEncSw, INPUT_PULLUP); }

Event poll() {
  uint32_t now = millis();
  bool down = (digitalRead(Pins::kEncSw) == LOW);   // pulled up, pressed = LOW

  // Accept a change only if the last accepted change was more than the debounce time ago.
  // (Verified on the bench: presses are clean, only the release bounces for < 200 us.)
  if (down != isDown && now - changedAt > Config::kButtonDebounceMs) {
    changedAt = now;
    isDown = down;
    if (down) {
      pressedAt = now;
      longPressFired = false;
    } else if (!longPressFired) {
      return Event::Click;
    }
  }

  if (isDown && !longPressFired && now - pressedAt > Config::kLongPressMs) {
    longPressFired = true;
    return Event::LongPress;
  }
  return Event::None;
}

}  // namespace Button
