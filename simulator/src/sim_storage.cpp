// SIMULATOR ONLY. Replaces drivers/flash_store.cpp: the two "files" are kept in memory and handed to the
// page, which stores them in the browser (localStorage) as hex text.
#include "drivers/flash_store.h"
#include <emscripten.h>
#include <string.h>
#include <vector>

EM_JS(int, simLoadSize, (const char* name), {
  const hex = Module.storageLoad ? Module.storageLoad(UTF8ToString(name)) : null;
  return hex ? hex.length / 2 : 0;
});

EM_JS(void, simLoad, (const char* name, uint8_t* out), {
  const hex = Module.storageLoad(UTF8ToString(name));
  for (let i = 0; i < hex.length / 2; i++) HEAPU8[out + i] = parseInt(hex.substr(2 * i, 2), 16);
});

EM_JS(void, simSave, (const char* name, const uint8_t* bytes, int length), {
  if (!Module.storageSave) return;
  let hex = "";
  for (let i = 0; i < length; i++) hex += HEAPU8[bytes + i].toString(16).padStart(2, "0");
  Module.storageSave(UTF8ToString(name), hex);
});

namespace {

struct StoredFile {
  const char* name;
  std::vector<uint8_t> bytes;

  void load() {
    bytes.resize((size_t)simLoadSize(name));
    if (!bytes.empty()) simLoad(name, bytes.data());
  }
  void store() { simSave(name, bytes.data(), (int)bytes.size()); }

  size_t read(size_t offset, uint8_t* buffer, size_t length) const {
    if (offset >= bytes.size()) return 0;
    const size_t n = length < bytes.size() - offset ? length : bytes.size() - offset;
    memcpy(buffer, bytes.data() + offset, n);
    return n;
  }
};

StoredFile save{"save", {}};
StoredFile log{"log", {}};

class SettingsBackend : public Persistence::Backend {
 public:
  size_t read(uint8_t* buffer, size_t capacity) override { return save.read(0, buffer, capacity); }
  bool write(const uint8_t* bytes, size_t length) override {
    save.bytes.assign(bytes, bytes + length);
    save.store();
    return true;
  }
};

class LogBackend : public SessionLog::Backend {
 public:
  size_t size() override { return log.bytes.size(); }
  size_t read(size_t offset, uint8_t* buffer, size_t length) override { return log.read(offset, buffer, length); }
  bool append(const uint8_t* bytes, size_t length) override {
    log.bytes.insert(log.bytes.end(), bytes, bytes + length);
    log.store();
    return true;
  }
  bool replace(const uint8_t* bytes, size_t length) override {
    log.bytes.assign(bytes, bytes + length);
    log.store();
    return true;
  }
};

SettingsBackend settingsInstance;
LogBackend logInstance;

}  // namespace

namespace FlashStore {

bool begin() {
  save.load();
  log.load();
  return true;
}

Persistence::Backend* backend() { return &settingsInstance; }
SessionLog::Backend* logBackend() { return &logInstance; }

}  // namespace FlashStore
