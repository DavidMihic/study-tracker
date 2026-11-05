#include "drivers/rtc_clock.h"
#include <Wire.h>
#include "config/config.h"
#include "config/pins.h"

namespace {

RTC_DS3231 chip;
bool available = false;
bool timeWasReset = false;
DateTime cached;               // last value read from the chip
bool newSecond = false;
uint32_t lastReadAt = 0;

}  // namespace

namespace Rtc {

bool begin() {
  Wire.setSDA(Pins::kRtcSda);   // must be set before Wire.begin()
  Wire.setSCL(Pins::kRtcScl);
  Wire.begin();

  available = chip.begin(&Wire);
  if (!available) return false;

  if (chip.lostPower()) {
    chip.adjust(DateTime(F(__DATE__), F(__TIME__)));
    timeWasReset = true;
  }
  cached = chip.now();
  newSecond = true;
  return true;
}

bool isAvailable() { return available; }
bool wasReset() { return timeWasReset; }

void poll() {
  if (!available) return;
  uint32_t ms = millis();
  if (ms - lastReadAt < Config::kRtcPollMs) return;
  lastReadAt = ms;

  DateTime reading = chip.now();
  if (reading.unixtime() != cached.unixtime()) {
    cached = reading;
    newSecond = true;
  }
}

bool secondChanged() {
  bool changed = newSecond;
  newSecond = false;
  return changed;
}

DateTime now() { return cached; }

void set(const DateTime& time) {
  if (!available) return;
  chip.adjust(time);
  cached = chip.now();
  newSecond = true;
  timeWasReset = false;
}

}  // namespace Rtc
