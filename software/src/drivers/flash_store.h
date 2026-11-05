// The flash storage backend: keeps the saved data in the flash sector that the arduino-pico core
// reserves for its EEPROM emulation. Writing erases and rewrites that 4 KB sector (about 50 to 100 ms,
// during which the processor is paused), so app/persistence keeps writes few.
#pragma once
#include "app/persistence.h"

namespace FlashStore {

// The backend object to give to Persistence::begin().
Persistence::Backend* backend();

}  // namespace FlashStore
