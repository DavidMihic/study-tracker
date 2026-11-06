// Saves and loads everything that must survive a power cut: subject totals, favourites, the last
// subject, the settings, and the study block that is running. It is the only place that knows WHEN to
// write; the flash itself is behind the small Backend interface (drivers/flash_store.cpp on the device,
// a plain array in the PC tests). Plain C++ (no Arduino).
//
// When it writes (flash wears out after ~100 000 erases, so writes are kept few):
//   saveNow()      right away: a block starts / pauses / resumes / stops.
//   requestSave()  after a setting, favourite or subject choice: waits until nothing changed for
//                  kSaveDelayMs, so turning the dial through a setting is ONE write, not dozens.
//   update()       called every loop: does the delayed write, and while a block runs it writes a
//                  checkpoint every kCheckpointSeconds so a power cut loses at most that much.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Persistence {

constexpr uint32_t kSaveDelayMs = 2000;
constexpr uint32_t kCheckpointSeconds = 10 * 60;

// Where the bytes live. read() gives back what write() stored last (length 0 = nothing stored).
class Backend {
 public:
  virtual ~Backend() = default;
  virtual size_t read(uint8_t* buffer, size_t capacity) = 0;
  virtual bool write(const uint8_t* bytes, size_t length) = 0;
};

// Time sources, passed in so this file has no hardware in it. nowUnix() may return 0 when the clock is
// not available.
struct Clock {
  uint32_t (*nowMs)();
  uint32_t (*nowUnix)();
};

void begin(Backend* backend, Clock clock);

// Reads the stored data into Progress. Returns false if nothing valid was stored: then everything stays
// at its defaults. (Call this early: the settings include the brightness.)
bool load();

// Brings back the study block that was running when the power was cut (paused). Call it after the clock
// has started, because the time that passed since then decides how much of the block counts (see
// Session::restorePaused). Does nothing if load() found no running block.
void restoreSession();

void saveNow();
void requestSave();
void update();

// True if restoreSession() brought back a study block (it is paused and waits for her to continue).
bool restoredSession();
bool restoredWasRunning();    // ... and it was running (not paused by her): the power cut stopped it
void acknowledgeRestored();   // the screen has shown the "restored" notice

// A restored block older than StudyRules::kAutoFinishSeconds is not shown: restoreSession() records it
// (counted time up to its last checkpoint) and the device starts on Home. This is true if that happened
// and the block was long enough to count; the Home screen shows a notice until it is acknowledged.
bool blockAutoFinished();
void acknowledgeAutoFinished();

}  // namespace Persistence
