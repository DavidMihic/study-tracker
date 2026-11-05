// DS3231 real-time clock. Reads the chip a few times per second and keeps the last value,
// so screens can ask for the time as often as they like without touching the I2C bus.
#pragma once
#include <Arduino.h>
#include <RTClib.h>

namespace Rtc {

// Sets up I2C and the chip. Returns false if the DS3231 does not answer.
// If the chip reports it lost power (no valid time), it is set to the firmware's compile time as a
// stopgap until the settings screen can set the clock; wasReset() then returns true.
bool begin();

bool isAvailable();
bool wasReset();

// Call every loop(). Reads the chip every Config::kRtcPollMs.
void poll();

// True once for every new second that poll() has seen. Use it to redraw the clock only when needed.
bool secondChanged();

// The most recent time read by poll().
DateTime now();

// Sets the chip's clock (settings screen, later).
void set(const DateTime& time);

}  // namespace Rtc
