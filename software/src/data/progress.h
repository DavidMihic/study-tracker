// What she has studied so far, per subject, plus the few "remembered" choices and settings.
// This is the in-memory copy; app/persistence loads it from flash at boot and saves it (see there for
// when). Nothing in here touches the flash itself.
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

// Used when loading from flash: sets all of a subject's numbers at once.
void loadStats(size_t subjectIndex, uint32_t totalSeconds, uint16_t sessions, bool favorite);

// True if the subject has any recorded time or sessions.
bool hasStats(size_t subjectIndex);

// Back to 0 time and 0 sessions. Favourites, the last subject and the settings are NOT touched.
void resetSubject(size_t subjectIndex);
void resetAllStats();

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

// Backlight level 0..255 as chosen in the settings (the Backlight driver clamps it to its limits).
uint8_t brightness();
void setBrightness(uint8_t level);
constexpr uint8_t kDefaultBrightness = 255;

// Length of one study block at which the nudge comes (minutes). Clamped to the limits in StudyRules.
uint16_t targetMinutes();
void setTargetMinutes(uint16_t minutes);

// Which sounds are on (all on by default). The nudge at the target time also has its colour change, so
// she can switch its sound off and still see it.
bool soundTurn();                 // tick for every click of the dial
bool soundPress();                // beeps for button presses and confirmations
bool soundNudge();                // the three beeps at the target time
bool soundRestore();              // the alert when power comes back during a study block
void setSoundTurn(bool on);
void setSoundPress(bool on);
void setSoundNudge(bool on);
void setSoundRestore(bool on);

}  // namespace Progress
