// SIMULATOR ONLY. Replaces drivers/rtc_clock.cpp: the clock runs along with millis(), so skipping time
// moves it forward too.
#include "drivers/rtc_clock.h"
#include "sim.h"

namespace {

uint32_t base = 0;             // clock time when millis() was 0
DateTime cached;
bool newSecond = false;

}  // namespace

namespace Sim {

uint32_t clockNow() { return base + millis() / 1000; }

void setClock(uint32_t unix) {
  base = unix - millis() / 1000;
  cached = DateTime(unix);
  newSecond = true;
}

}  // namespace Sim

namespace Rtc {

bool begin() {
  cached = DateTime(Sim::clockNow());
  newSecond = true;
  return true;
}

bool isAvailable() { return true; }
bool wasReset() { return false; }

void poll() {
  const uint32_t now = Sim::clockNow();
  if (now != cached.unixtime()) {
    cached = DateTime(now);
    newSecond = true;
  }
}

bool secondChanged() {
  const bool changed = newSecond;
  newSecond = false;
  return changed;
}

DateTime now() { return cached; }

void set(const DateTime& time) { Sim::setClock(time.unixtime()); }

}  // namespace Rtc
