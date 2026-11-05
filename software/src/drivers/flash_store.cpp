#include "drivers/flash_store.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "data/save_format.h"

namespace {

// The emulated EEPROM is one flash sector of 4096 bytes; the saved data is smaller than that.
constexpr size_t kSectorBytes = 4096;
static_assert(SaveFormat::kMaxBytes <= kSectorBytes, "saved data no longer fits into one flash sector");

class EepromBackend : public Persistence::Backend {
 public:
  size_t read(uint8_t* buffer, size_t capacity) override {
    EEPROM.begin(kSectorBytes);
    size_t n = capacity < kSectorBytes ? capacity : kSectorBytes;
    for (size_t i = 0; i < n; i++) buffer[i] = EEPROM.read((int)i);
    return n;   // parse() finds out whether this is valid data or blank flash
  }

  bool write(const uint8_t* bytes, size_t length) override {
    EEPROM.begin(kSectorBytes);
    for (size_t i = 0; i < length; i++) EEPROM.write((int)i, bytes[i]);
    return EEPROM.commit();   // only erases/programs if a byte really changed
  }
};

EepromBackend instance;

}  // namespace

namespace FlashStore {

Persistence::Backend* backend() { return &instance; }

}  // namespace FlashStore
