#include "data/progress.h"
#include "app/semester.h"
#include "config/study_rules.h"
#include "data/subjects.h"

namespace {

SubjectStats table[Subjects::kMaxSubjects] = {};
uint16_t lastSubjectId = 0;                       // 0 = none chosen yet (ids are never 0)
uint8_t semester = StudyRules::kDefaultSemester;
uint16_t target = StudyRules::kDefaultTargetMinutes;
bool turnSound = true, pressSound = true, nudgeSound = true, restoreSound = true;
uint8_t brightnessLevel = Progress::kDefaultBrightness;

}  // namespace

namespace Progress {

void reset() {
  for (size_t i = 0; i < Subjects::kMaxSubjects; i++) table[i] = SubjectStats{0, 0, false};
  lastSubjectId = 0;
  semester = StudyRules::kDefaultSemester;
  target = StudyRules::kDefaultTargetMinutes;
  turnSound = pressSound = nudgeSound = restoreSound = true;
  brightnessLevel = kDefaultBrightness;
}

const SubjectStats& stats(size_t subjectIndex) { return table[subjectIndex]; }

void recordSession(size_t subjectIndex, uint32_t seconds) {
  table[subjectIndex].totalSeconds += seconds;
  table[subjectIndex].sessions++;
}

void loadStats(size_t subjectIndex, uint32_t totalSeconds, uint16_t sessions, bool favorite) {
  table[subjectIndex] = SubjectStats{totalSeconds, sessions, favorite};
}

bool hasStats(size_t subjectIndex) {
  return table[subjectIndex].totalSeconds > 0 || table[subjectIndex].sessions > 0;
}

void resetSubject(size_t subjectIndex) {
  table[subjectIndex].totalSeconds = 0;
  table[subjectIndex].sessions = 0;
}

void resetAllStats() {
  for (size_t i = 0; i < Subjects::kMaxSubjects; i++) resetSubject(i);
}

void setFavorite(size_t subjectIndex, bool favorite) { table[subjectIndex].favorite = favorite; }

bool toggleFavorite(size_t subjectIndex) {
  table[subjectIndex].favorite = !table[subjectIndex].favorite;
  return table[subjectIndex].favorite;
}

int lastSubjectIndex() {
  if (lastSubjectId == 0) return -1;
  return Subjects::indexOfId(lastSubjectId);
}

void setLastSubject(size_t subjectIndex) { lastSubjectId = Subjects::at(subjectIndex).id; }

uint8_t currentSemester() { return semester; }

void setCurrentSemester(uint8_t value) {
  if (value < Semesters::kFirst) value = Semesters::kFirst;
  if (value > Semesters::kLast) value = Semesters::kLast;
  semester = value;
}

uint16_t targetMinutes() { return target; }

void setTargetMinutes(uint16_t minutes) {
  if (minutes < StudyRules::kMinTargetMinutes) minutes = StudyRules::kMinTargetMinutes;
  if (minutes > StudyRules::kMaxTargetMinutes) minutes = StudyRules::kMaxTargetMinutes;
  target = minutes;
}

uint8_t brightness() { return brightnessLevel; }
void setBrightness(uint8_t level) { brightnessLevel = level; }

bool soundTurn() { return turnSound; }
bool soundPress() { return pressSound; }
bool soundNudge() { return nudgeSound; }
bool soundRestore() { return restoreSound; }
void setSoundTurn(bool on) { turnSound = on; }
void setSoundPress(bool on) { pressSound = on; }
void setSoundNudge(bool on) { nudgeSound = on; }
void setSoundRestore(bool on) { restoreSound = on; }

}  // namespace Progress
