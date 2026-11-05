// What she has studied so far, per subject, plus the few "remembered" choices.
// Build step 2 keeps this in RAM only (lost at power-off). Step 3 will load it from flash at boot and
// save it when a session ends, without changing this interface.
// Plain C++ (no Arduino), so the PC tests can use it.
#pragma once
#include <stddef.h>
#include <stdint.h>

struct SubjectStats {
  uint32_t totalSeconds;
  uint16_t sessions;
  bool favorite;
};

namespace Progress {

// Forget everything (also used by the tests).
void reset();

// Stats of the subject at this position in the Subjects list.
const SubjectStats& stats(size_t subjectIndex);

// Adds one finished study block.
void recordSession(size_t subjectIndex, uint32_t seconds);

void setFavorite(size_t subjectIndex, bool favorite);
// Flips the favourite flag and returns the new value.
bool toggleFavorite(size_t subjectIndex);

// The subject that is selected when the device starts (the one she studied last).
// Stored as the stable id; returns the list position, or -1 if nothing was chosen yet
// or the subject no longer exists.
int lastSubjectIndex();
void setLastSubject(size_t subjectIndex);

// Her current semester (1..12, see app/semester.h). Decides which subjects the main list shows first.
// Values outside the range are clamped.
uint8_t currentSemester();
void setCurrentSemester(uint8_t semester);

// Length of one study block at which the nudge comes (minutes). Clamped to the limits in StudyRules.
uint16_t targetMinutes();
void setTargetMinutes(uint16_t minutes);

}  // namespace Progress
