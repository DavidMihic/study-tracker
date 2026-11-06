#include "drivers/flash_store.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include "data/save_format.h"

namespace {

constexpr const char* kSavePath = "/save.bin";
constexpr const char* kSaveTempPath = "/save.tmp";
constexpr const char* kLogPath = "/sessions.log";
constexpr const char* kLogTempPath = "/sessions.tmp";

// Data written by the first firmware versions lived in the EEPROM-emulation sector. It is read once if
// there is no /save.bin yet, so the totals and settings survive the change to LittleFS.
constexpr size_t kLegacySectorBytes = 4096;

bool mounted = false;

// Writes bytes to a temporary file and renames it over the real one. rename() is atomic in LittleFS.
bool replaceFile(const char* path, const char* tempPath, const uint8_t* bytes, size_t length) {
  if (!mounted) return false;
  File file = LittleFS.open(tempPath, "w");
  if (!file) return false;
  bool ok = (length == 0) || (file.write(bytes, length) == length);
  file.close();
  if (!ok) { LittleFS.remove(tempPath); return false; }
  LittleFS.remove(path);   // the rename below fails on some versions if the target exists
  return LittleFS.rename(tempPath, path);
}

class SettingsBackend : public Persistence::Backend {
 public:
  size_t read(uint8_t* buffer, size_t capacity) override {
    if (!mounted) return 0;
    if (LittleFS.exists(kSavePath)) {
      File file = LittleFS.open(kSavePath, "r");
      if (!file) return 0;
      size_t n = file.read(buffer, capacity);
      file.close();
      return n;
    }
    // First start after the change to LittleFS: take over the old EEPROM sector (parse() checks it).
    EEPROM.begin(kLegacySectorBytes);
    size_t n = capacity < kLegacySectorBytes ? capacity : kLegacySectorBytes;
    for (size_t i = 0; i < n; i++) buffer[i] = EEPROM.read((int)i);
    return n;
  }

  bool write(const uint8_t* bytes, size_t length) override {
    return replaceFile(kSavePath, kSaveTempPath, bytes, length);
  }
};

class LogBackend : public SessionLog::Backend {
 public:
  size_t size() override {
    if (!mounted || !LittleFS.exists(kLogPath)) return 0;
    File file = LittleFS.open(kLogPath, "r");
    if (!file) return 0;
    size_t n = file.size();
    file.close();
    return n;
  }

  size_t read(size_t offset, uint8_t* buffer, size_t length) override {
    if (!mounted) return 0;
    File file = LittleFS.open(kLogPath, "r");
    if (!file) return 0;
    size_t n = 0;
    if (file.seek(offset)) n = file.read(buffer, length);
    file.close();
    return n;
  }

  bool append(const uint8_t* bytes, size_t length) override {
    if (!mounted) return false;
    File file = LittleFS.open(kLogPath, "a");
    if (!file) return false;
    bool ok = file.write(bytes, length) == length;
    file.close();   // close() commits the data to flash
    return ok;
  }

  bool replace(const uint8_t* bytes, size_t length) override {
    return replaceFile(kLogPath, kLogTempPath, bytes, length);
  }
};

SettingsBackend settingsInstance;
LogBackend logInstance;

}  // namespace

namespace FlashStore {

bool begin() {
  mounted = LittleFS.begin();   // formats the file system on the first start (auto-format is the default)
  if (mounted) {
    // A cut between the remove() and the rename() in replaceFile() leaves only the finished temp file:
    // finish the swap. A temp file next to a real one is a half-written leftover: delete it.
    const char* pairs[][2] = {{kSavePath, kSaveTempPath}, {kLogPath, kLogTempPath}};
    for (auto& pair : pairs) {
      if (!LittleFS.exists(pair[1])) continue;
      if (LittleFS.exists(pair[0])) LittleFS.remove(pair[1]);
      else LittleFS.rename(pair[1], pair[0]);
    }
  }
  return mounted;
}

Persistence::Backend* backend() { return &settingsInstance; }
SessionLog::Backend* logBackend() { return &logInstance; }

}  // namespace FlashStore
