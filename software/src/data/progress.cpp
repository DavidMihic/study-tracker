#include "data/progress.h"
#include "app/semester.h"
#include "config/study_rules.h"
#include "data/subjects.h"

namespace {

SubjectStats table[Subjects::kMaxSubjects] = {};
uint16_t lastSubjectId = 0;                       // 0 = none chosen yet (ids are never 0)
uint8_t semester = StudyRules::kDefaultSemester;
uint16_t target = StudyRules::kDefaultTargetMinutes;

}  // namespace

namespace Progress {

void reset() {
  for (size_t i = 0; i < Subjects::kMaxSubjects; i++) table[i] = SubjectStats{0, 0, false};
  lastSubjectId = 0;
  semester = StudyRules::kDefaultSemester;
  target = StudyRules::kDefaultTargetMinutes;
}

const SubjectStats& stats(size_t subjectIndex) { return table[subjectIndex]; }

void recordSession(size_t subjectIndex, uint32_t seconds) {
  table[subjectIndex].totalSeconds += seconds;
  table[subjectIndex].sessions++;
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

}  // namespace Progress
