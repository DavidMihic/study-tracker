// The study session logic: study block (count-up, can be paused, nudge at the target, never stops by
// itself) followed by a break (count-up). It only does arithmetic on unix times that the caller passes
// in, so it has no hardware in it and the PC tests can use it. Plain C++ (no Arduino).
#pragma once
#include <stddef.h>
#include <stdint.h>

class Session {
 public:
  enum class Phase : uint8_t { Idle, Studying, Break };

  Phase phase() const { return phase_; }

  // Starts a study block on this subject (position in the Subjects list).
  void startStudy(uint32_t nowUnix, size_t subjectIndex, uint32_t targetSeconds);

  // Paused time does not count. Both do nothing outside a study block or if already in that state.
  void pause(uint32_t nowUnix);
  void resume(uint32_t nowUnix);
  bool isPaused() const { return phase_ == Phase::Studying && !running_; }

  // Counted seconds of the current phase (0 when idle or if the clock went backwards).
  uint32_t elapsedSeconds(uint32_t nowUnix) const;

  // True exactly once, the first time the study block reaches its target. The block keeps running.
  bool nudgeDue(uint32_t nowUnix);
  bool targetReached(uint32_t nowUnix) const;

  // Changes the target of the running block (from the settings). A nudge that already came is not repeated.
  void setTargetSeconds(uint32_t targetSeconds) { targetSeconds_ = targetSeconds; }

  // Ends the study block (also when paused) and starts the break. Returns the counted length in seconds.
  uint32_t endStudy(uint32_t nowUnix);

  // True if a block of this length is long enough to be counted (see StudyRules).
  static bool isCounted(uint32_t seconds);

  // Break is over (back to the home screen or into a new block).
  void endBreak();

  // For saving to flash. A snapshot is only meaningful while a study block is running or paused.
  struct Snapshot {
    bool active;                 // false when idle or in a break: nothing to resume
    size_t subjectIndex;
    uint32_t accumulatedSeconds;
    uint32_t runStartUnix;
    bool running;
    uint32_t checkpointUnix;
    uint32_t startedUnix;        // when the block was first started (pauses and power cuts do not change it)
  };
  Snapshot snapshot() const;

  // When the current study block was first started (for the session log).
  uint32_t startedUnix() const { return startedUnix_; }

  // Remembers "the device is running right now" (called before saving; only matters while running).
  void checkpoint(uint32_t nowUnix);

  // Brings back a block after a power cut: it comes back PAUSED. If it was running, the time up to the
  // last checkpoint counts for sure. If the device is back within StudyRules::kResumeTrustSeconds of
  // that checkpoint, the time until now counts too (the interruption was short); otherwise the time
  // after the checkpoint is lost. nowUnix = 0 means the clock is not available (counts to the checkpoint).
  void restorePaused(const Snapshot& snapshot, uint32_t targetSeconds, uint32_t nowUnix);

  size_t subjectIndex() const { return subjectIndex_; }
  uint32_t targetSeconds() const { return targetSeconds_; }

 private:
  Phase phase_ = Phase::Idle;
  size_t subjectIndex_ = 0;
  uint32_t accumulated_ = 0;   // seconds counted before the current running stretch
  uint32_t runStart_ = 0;      // when the current running stretch began
  bool running_ = false;       // false while paused
  uint32_t targetSeconds_ = 0;
  bool nudgeFired_ = false;
  uint32_t checkpoint_ = 0;    // last time the block was known to be running
  uint32_t startedUnix_ = 0;   // when the block was first started
};
