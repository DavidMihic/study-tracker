// SIMULATOR ONLY. What the replacement drivers need from each other.
#pragma once
#include <stdint.h>

namespace Sim {

// Time. The page passes its own millisecond counter in; skipping moves the device's time forward.
void setHostMs(double hostMs);
void skipMs(uint32_t ms);

// Input from the page: dial clicks waiting to be read, and the state of the push switch.
void turn(int clicks);
void setButton(bool down);

// Backlight level (0..255) last written by the firmware.
int backlight();

// Real time clock, as the local wall-clock time in unix seconds.
void setClock(uint32_t unix);
uint32_t clockNow();

}  // namespace Sim
