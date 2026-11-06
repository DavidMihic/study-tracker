// Flash storage on the device. Everything lives in a LittleFS file system in the board's 8 MB flash chip
// (size set by board_build.filesystem_size in platformio.ini). LittleFS writes copy-on-write and
// renames atomically, so a power cut in the middle of a save leaves either the old or the new file,
// never a wiped one.
//   /save.bin     settings, totals, favourites, the running block (replaced as a whole at each save)
//   /sessions.log the session log (appended, one record per finished block)
// Each save erases a 4 KB flash block and the processor pauses about 50 to 100 ms, so app/persistence
// keeps writes few.
#pragma once
#include "app/persistence.h"
#include "data/session_log.h"

namespace FlashStore {

// Mounts the file system (formats it on the very first start). Call once before using the backends.
// Returns false if it could not be mounted; the backends then store nothing and the device runs from
// defaults.
bool begin();

// The backend objects to give to Persistence::begin() and SessionLog::begin().
Persistence::Backend* backend();
SessionLog::Backend* logBackend();

}  // namespace FlashStore
