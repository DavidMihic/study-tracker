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

// The gentle nudge comes at this length; the session keeps running. Adjustable in the settings.
constexpr uint16_t kDefaultTargetMinutes = 25;
constexpr uint16_t kMinTargetMinutes     = 5;
constexpr uint16_t kMaxTargetMinutes     = 120;
constexpr uint16_t kTargetStepMinutes    = 5;

}  // namespace StudyRules
