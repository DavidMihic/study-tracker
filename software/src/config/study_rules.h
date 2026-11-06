// Rules of the study tracker that are not about hardware. Plain C++ (no Arduino), so the
// PC tests can use it. Change a value here and the whole firmware follows.
#pragma once
#include <stdint.h>

namespace StudyRules {

// Medical studies in Croatia take six years (12 semesters, see app/semester.h).
constexpr uint8_t kFirstStudyYear   = 1;
constexpr uint8_t kLastStudyYear    = 6;

// The semester she is in until she changes it (David: semester 5, winter of year 3).
constexpr uint8_t kDefaultSemester  = 5;

// A study block shorter than this is thrown away instead of being counted (an accidental click
// would otherwise add a "session" of a few seconds). Set to 0 to count everything.
constexpr uint32_t kMinCountedSessionSeconds = 60;

// After a power cut a running block is restored (paused). If the device is back no later than this
// after its last checkpoint, the block is assumed to have been running the whole time and the time that
// passed counts; later than that, only the time up to the last checkpoint counts.
constexpr uint32_t kResumeTrustSeconds = 15 * 60;

// A block restored after a power cut that has been waiting longer than this (measured from its last
// checkpoint) is finished automatically at boot: the time counted up to the checkpoint is recorded and the
// device starts on the home screen, instead of showing an old paused block. Nothing is thrown away.
constexpr uint32_t kAutoFinishSeconds = 3 * 60 * 60;

// The study day for the day/week statistics starts at this hour, not at midnight: a block that begins
// on Wednesday evening and runs past midnight still belongs to Wednesday. A block counts for the study
// day in which it STARTED. (Only matters if she studies straight through this hour.)
constexpr uint8_t kStudyDayStartHour = 5;

// The gentle nudge comes at this length; the session keeps running. Adjustable in the settings.
constexpr uint16_t kDefaultTargetMinutes = 25;
constexpr uint16_t kMinTargetMinutes     = 5;
constexpr uint16_t kMaxTargetMinutes     = 120;
constexpr uint16_t kTargetStepMinutes    = 5;

}  // namespace StudyRules
