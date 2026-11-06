// PC tests on the REAL subject list (src/data/subjects.cpp), unlike the other tests, which use a small
// fixed list. They catch typing mistakes when the curriculum is edited, and check that the screens'
// lists and the saved data work at full size. Run with:  pio test -e native_curriculum
#include <stdio.h>
#include <string.h>
#include <unity.h>
#include <vector>

#include "app/persistence.h"
#include "app/stats.h"
#include "app/subject_list.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "data/save_format.h"
#include "data/session_log.h"
#include "data/subjects.h"

using Subjects::at;
using Subjects::count;

namespace {

class MemoryBackend : public Persistence::Backend {
 public:
  std::vector<uint8_t> bytes;
  size_t read(uint8_t* buffer, size_t capacity) override {
    size_t n = bytes.size() < capacity ? bytes.size() : capacity;
    if (n) memcpy(buffer, bytes.data(), n);
    return n;
  }
  bool write(const uint8_t* data, size_t length) override { bytes.assign(data, data + length); return true; }
};

class MemoryLogBackend : public SessionLog::Backend {
 public:
  std::vector<uint8_t> bytes;
  size_t size() override { return bytes.size(); }
  size_t read(size_t offset, uint8_t* buffer, size_t length) override {
    if (offset >= bytes.size()) return 0;
    size_t n = bytes.size() - offset < length ? bytes.size() - offset : length;
    memcpy(buffer, bytes.data() + offset, n);
    return n;
  }
  bool append(const uint8_t* data, size_t length) override { bytes.insert(bytes.end(), data, data + length); return true; }
  bool replace(const uint8_t* data, size_t length) override { bytes.assign(data, data + length); return true; }
};

MemoryBackend flash;
MemoryLogBackend logFlash;
uint32_t gMs = 0, gUnix = 1700000000;
uint32_t fakeMs() { return gMs; }
uint32_t fakeUnix() { return gUnix; }

// Checks that the text is well-formed UTF-8 (a broken byte shows up as garbage on the display).
bool isValidUtf8(const char* s) {
  const unsigned char* p = (const unsigned char*)s;
  while (*p) {
    int extra = (*p < 0x80) ? 0 : ((*p & 0xE0) == 0xC0) ? 1 : ((*p & 0xF0) == 0xE0) ? 2 : ((*p & 0xF8) == 0xF0) ? 3 : -1;
    if (extra < 0) return false;
    p++;
    for (int i = 0; i < extra; i++, p++) {
      if ((*p & 0xC0) != 0x80) return false;
    }
  }
  return true;
}

constexpr uint8_t kHalves[] = {Subjects::kWinter, Subjects::kSummer};

}  // namespace

void setUp() {
  Progress::reset();
  flash.bytes.clear();
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  Persistence::begin(&flash, {fakeMs, fakeUnix});
}
void tearDown() {}

// ---------------------------------------------------------------- the data itself
void test_ids_are_unique_and_not_zero() {
  for (size_t i = 0; i < count(); i++) {
    TEST_ASSERT_NOT_EQUAL(0, at(i).id);
    for (size_t j = i + 1; j < count(); j++) TEST_ASSERT_NOT_EQUAL(at(i).id, at(j).id);
  }
}

void test_list_fits_the_tables() {
  TEST_ASSERT_TRUE(count() > 0);
  TEST_ASSERT_TRUE(count() <= Subjects::kMaxSubjects);
}

void test_every_field_is_in_range() {
  for (size_t i = 0; i < count(); i++) {
    const Subjects::Subject& s = at(i);
    TEST_ASSERT_MESSAGE(s.year >= StudyRules::kFirstStudyYear && s.year <= StudyRules::kLastStudyYear, s.name);
    TEST_ASSERT_MESSAGE(s.semesters >= Subjects::kWinter && s.semesters <= Subjects::kBoth, s.name);
  }
}

void test_names_are_clean() {
  for (size_t i = 0; i < count(); i++) {
    const char* name = at(i).name;
    const size_t length = strlen(name);
    TEST_ASSERT_MESSAGE(length > 0, "empty name");
    TEST_ASSERT_MESSAGE(length <= Subjects::kMaxNameBytes, name);
    TEST_ASSERT_MESSAGE(isValidUtf8(name), name);
    TEST_ASSERT_MESSAGE(name[0] != ' ' && name[length - 1] != ' ', name);     // no stray spaces at the ends
    TEST_ASSERT_MESSAGE(strstr(name, "  ") == nullptr, name);                  // or doubled inside
    TEST_ASSERT_MESSAGE(strstr(name, "\xEF\xBF\xBD") == nullptr, name);        // U+FFFD = a lost character
    for (size_t k = 0; k < length; k++) TEST_ASSERT_MESSAGE((unsigned char)name[k] >= 0x20, name);
  }
}

// The statistics and the Studied list show only the name, so two subjects must never look the same.
void test_names_are_unique() {
  for (size_t i = 0; i < count(); i++) {
    for (size_t j = i + 1; j < count(); j++) TEST_ASSERT_MESSAGE(strcmp(at(i).name, at(j).name) != 0, at(i).name);
  }
}

void test_every_semester_has_regular_subjects() {
  for (uint8_t year = StudyRules::kFirstStudyYear; year <= StudyRules::kLastStudyYear; year++) {
    for (uint8_t half : kHalves) {
      size_t list[Subjects::kMaxSubjects];
      size_t n = SubjectList::buildMain({year, half}, list, Subjects::kMaxSubjects);
      char message[48];
      snprintf(message, sizeof message, "year %u half %u has no subjects", year, half);
      TEST_ASSERT_MESSAGE(n > 0, message);
    }
  }
}

// ---------------------------------------------------------------- the lists the screens build
void test_every_elective_is_in_the_folder_of_each_of_its_semesters() {
  for (size_t i = 0; i < count(); i++) {
    if (!at(i).elective) continue;
    for (uint8_t half : kHalves) {
      size_t list[Subjects::kMaxSubjects];
      size_t n = SubjectList::buildElectives({at(i).year, half}, list, Subjects::kMaxSubjects);
      const bool expected = (at(i).semesters & half) != 0;
      TEST_ASSERT_MESSAGE((SubjectList::find(list, n, i) >= 0) == expected, at(i).name);
    }
  }
}

void test_an_elective_is_in_no_other_year() {
  for (size_t i = 0; i < count(); i++) {
    if (!at(i).elective) continue;
    for (uint8_t year = StudyRules::kFirstStudyYear; year <= StudyRules::kLastStudyYear; year++) {
      if (year == at(i).year) continue;
      size_t list[Subjects::kMaxSubjects];
      size_t n = SubjectList::buildElectives({year, Subjects::kWinter}, list, Subjects::kMaxSubjects);
      TEST_ASSERT_MESSAGE(SubjectList::find(list, n, i) < 0, at(i).name);
    }
  }
}

void test_unstudied_electives_stay_out_of_the_main_list_until_studied_or_favourited() {
  for (size_t i = 0; i < count(); i++) {
    if (!at(i).elective) continue;
    const SubjectList::View view = {at(i).year, (at(i).semesters & Subjects::kWinter) ? Subjects::kWinter : Subjects::kSummer};
    size_t list[Subjects::kMaxSubjects];
    size_t n = SubjectList::buildMain(view, list, Subjects::kMaxSubjects);
    TEST_ASSERT_MESSAGE(SubjectList::find(list, n, i) < 0, at(i).name);

    Progress::setFavorite(i, true);
    n = SubjectList::buildMain(view, list, Subjects::kMaxSubjects);
    TEST_ASSERT_MESSAGE(SubjectList::find(list, n, i) == 0, at(i).name);   // favourites go first
    Progress::setFavorite(i, false);
  }
}

// A subject offered in both semesters is ONE subject: the same index in both lists, one set of totals.
void test_a_two_semester_subject_is_the_same_entry_in_both_lists() {
  bool seen = false;
  for (size_t i = 0; i < count(); i++) {
    if (at(i).elective || at(i).semesters != Subjects::kBoth) continue;
    seen = true;
    size_t winter[Subjects::kMaxSubjects], summer[Subjects::kMaxSubjects];
    size_t nw = SubjectList::buildMain({at(i).year, Subjects::kWinter}, winter, Subjects::kMaxSubjects);
    size_t ns = SubjectList::buildMain({at(i).year, Subjects::kSummer}, summer, Subjects::kMaxSubjects);
    TEST_ASSERT_MESSAGE(SubjectList::find(winter, nw, i) >= 0, at(i).name);
    TEST_ASSERT_MESSAGE(SubjectList::find(summer, ns, i) >= 0, at(i).name);
  }
  TEST_ASSERT_TRUE(seen);
}

void test_studied_subjects_come_first_most_time_first() {
  const SubjectList::View view = {1, Subjects::kWinter};
  size_t before[Subjects::kMaxSubjects];
  size_t n = SubjectList::buildMain(view, before, Subjects::kMaxSubjects);
  TEST_ASSERT_TRUE(n >= 3);
  Progress::recordSession(before[n - 1], 600);
  Progress::recordSession(before[n - 2], 1200);

  size_t after[Subjects::kMaxSubjects];
  TEST_ASSERT_EQUAL(n, SubjectList::buildMain(view, after, Subjects::kMaxSubjects));
  TEST_ASSERT_EQUAL(before[n - 2], after[0]);
  TEST_ASSERT_EQUAL(before[n - 1], after[1]);
  TEST_ASSERT_EQUAL(before[0], after[2]);              // the rest keep the order of the curriculum
}

void test_studied_list_shows_exactly_the_subjects_with_time() {
  size_t list[Subjects::kMaxSubjects];
  TEST_ASSERT_EQUAL(0, SubjectList::buildStudied(list, Subjects::kMaxSubjects));
  Progress::recordSession(0, 100);
  Progress::recordSession(count() - 1, 100);
  TEST_ASSERT_EQUAL(2, SubjectList::buildStudied(list, Subjects::kMaxSubjects));
}

// ---------------------------------------------------------------- full-size saving
static void studyEverything() {
  for (size_t i = 0; i < count(); i++) {
    Progress::recordSession(i, 600 + (uint32_t)i * 7);
    Progress::recordSession(i, 60 + (uint32_t)i);
    if (i % 3 == 0) Progress::setFavorite(i, true);
  }
  Progress::setLastSubject(count() - 1);
}

void test_data_for_every_subject_survives_a_reboot() {
  studyEverything();
  Persistence::saveNow();

  Progress::reset();
  TEST_ASSERT_TRUE(Persistence::load());
  for (size_t i = 0; i < count(); i++) {
    TEST_ASSERT_EQUAL_MESSAGE(660 + i * 8, Progress::stats(i).totalSeconds, at(i).name);
    TEST_ASSERT_EQUAL_MESSAGE(2, Progress::stats(i).sessions, at(i).name);
    TEST_ASSERT_EQUAL_MESSAGE(i % 3 == 0, Progress::stats(i).favorite, at(i).name);
  }
  TEST_ASSERT_EQUAL((int)(count() - 1), Progress::lastSubjectIndex());
}

void test_save_file_is_small_and_fits_its_buffer() {
  studyEverything();
  Persistence::saveNow();
  TEST_ASSERT_TRUE(flash.bytes.size() > 0);
  TEST_ASSERT_TRUE(flash.bytes.size() <= SaveFormat::kMaxBytes);
  TEST_ASSERT_TRUE(flash.bytes.size() < 4096);     // one flash sector; the file system has 1 MB
}

// Removing a subject from the list drops only its record; the others keep their data.
void test_a_subject_removed_from_the_list_does_not_disturb_the_others() {
  studyEverything();
  Persistence::saveNow();
  SaveFormat::Data data = {};
  TEST_ASSERT_TRUE(SaveFormat::parse(flash.bytes.data(), flash.bytes.size(), data));
  TEST_ASSERT_EQUAL(count(), data.subjectCount);
  data.subjects[0].id = 60000;                      // an id that is not in the list (a deleted subject)
  uint8_t bytes[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(data, bytes, sizeof bytes);
  flash.bytes.assign(bytes, bytes + n);

  Progress::reset();
  TEST_ASSERT_TRUE(Persistence::load());
  TEST_ASSERT_EQUAL(0, Progress::stats(0).totalSeconds);
  TEST_ASSERT_EQUAL(660 + 1 * 8, Progress::stats(1).totalSeconds);
  TEST_ASSERT_EQUAL(660 + (count() - 1) * 8, Progress::stats(count() - 1).totalSeconds);
}

void test_statistics_cover_every_subject() {
  const uint32_t day = SessionLog::studyDay(gUnix);
  for (size_t i = 0; i < count(); i++) {
    TEST_ASSERT_TRUE(SessionLog::append({gUnix, 600 + (uint32_t)i, at(i).id}));
  }
  Stats::SubjectTotal rows[Subjects::kMaxSubjects];
  size_t n = Stats::perSubject({day, day}, rows, Subjects::kMaxSubjects);
  TEST_ASSERT_EQUAL(count(), n);
  TEST_ASSERT_EQUAL(at(count() - 1).id, rows[0].subjectId);     // the longest block first
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ids_are_unique_and_not_zero);
  RUN_TEST(test_list_fits_the_tables);
  RUN_TEST(test_every_field_is_in_range);
  RUN_TEST(test_names_are_clean);
  RUN_TEST(test_names_are_unique);
  RUN_TEST(test_every_semester_has_regular_subjects);
  RUN_TEST(test_every_elective_is_in_the_folder_of_each_of_its_semesters);
  RUN_TEST(test_an_elective_is_in_no_other_year);
  RUN_TEST(test_unstudied_electives_stay_out_of_the_main_list_until_studied_or_favourited);
  RUN_TEST(test_a_two_semester_subject_is_the_same_entry_in_both_lists);
  RUN_TEST(test_studied_subjects_come_first_most_time_first);
  RUN_TEST(test_studied_list_shows_exactly_the_subjects_with_time);
  RUN_TEST(test_data_for_every_subject_survives_a_reboot);
  RUN_TEST(test_save_file_is_small_and_fits_its_buffer);
  RUN_TEST(test_a_subject_removed_from_the_list_does_not_disturb_the_others);
  RUN_TEST(test_statistics_cover_every_subject);
  return UNITY_END();
}
