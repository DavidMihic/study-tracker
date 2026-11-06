#ifdef DUMMY_DATA

#include "debug/dummy_data.h"
#include "app/semester.h"
#include "app/subject_list.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "data/session_log.h"
#include "data/subjects.h"

namespace DummyData {

namespace {

constexpr int kDays = 40;
constexpr size_t kPoolSize = 5;   // blocks are spread over this many subjects

// Small deterministic pseudo-random numbers (a block of a given day is always the same).
class Random {
 public:
  explicit Random(uint32_t seed) : state_(seed * 2654435761u + 12345u) {}
  uint32_t next() { state_ = state_ * 1664525u + 1013904223u; return state_ >> 8; }
 private:
  uint32_t state_;
};

}  // namespace

void seedIfEmpty(uint32_t nowUnix) {
  if (nowUnix == 0 || SessionLog::count() > 0) return;

  // Subjects of her current semester (the same list the picker shows), the first few of them.
  uint8_t semester = Progress::currentSemester();
  SubjectList::View view = {Semesters::yearOf(semester), Semesters::halfOf(semester)};
  size_t list[Subjects::kMaxSubjects];
  size_t found = SubjectList::buildMain(view, list, Subjects::kMaxSubjects);
  uint16_t pool[kPoolSize];
  size_t poolSize = 0;
  for (size_t i = 0; i < found && poolSize < kPoolSize; i++) pool[poolSize++] = Subjects::at(list[i]).id;
  if (poolSize == 0) pool[poolSize++] = Subjects::at(0).id;

  const uint32_t today = SessionLog::studyDay(nowUnix);
  const uint32_t shift = (uint32_t)StudyRules::kStudyDayStartHour * 3600u;

  for (int back = kDays - 1; back >= 0 && (uint32_t)back <= today; back--) {
    const uint32_t day = today - (uint32_t)back;
    const uint32_t dayStart = day * 86400u + shift;
    Random random(day);

    const bool weekend = SessionLog::weekdayOfStudyDay(day) == 0 || SessionLog::weekdayOfStudyDay(day) == 6;
    const uint32_t blocks = random.next() % (weekend ? 3 : 5);          // 0 to 2 / 0 to 4 blocks
    for (uint32_t b = 0; b < blocks; b++) {
      // Start between 08:00 and 02:00 the next night (study day offsets 3 h to 21 h), so some blocks
      // run past midnight and still count for their own day.
      uint32_t start = dayStart + (3 + random.next() % 19) * 3600u + (random.next() % 60) * 60u;
      uint32_t seconds = 1200u + random.next() % 4500u;                  // 20 to 95 minutes
      uint16_t subject = pool[random.next() % poolSize];
      if (start + seconds > nowUnix) continue;                           // nothing in the future
      SessionLog::append({start, seconds, subject});
    }
  }
}

}  // namespace DummyData

#endif  // DUMMY_DATA
