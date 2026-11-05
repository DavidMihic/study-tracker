// PC tests for the logic that has no hardware in it: semester numbers, subject ordering, progress,
// the study session and the text formatting. Run with:  pio test -e native
// (They use the fixed subject list in test/support/test_subjects.cpp.)
#include <string.h>
#include <unity.h>

#include "app/date_edit.h"
#include "app/semester.h"
#include "app/session.h"
#include "app/subject_list.h"
#include "data/progress.h"
#include "data/subjects.h"
#include "ui/format.h"

using SubjectList::View;

// Subject positions in the fixture list
enum : size_t { kA1 = 0, kA2 = 1, kW1 = 2, kS1 = 3, kB1 = 4, kE1 = 5, kE2 = 6, kE3 = 7 };

void setUp() { Progress::reset(); }
void tearDown() {}

// ---------------------------------------------------------------- semesters
void test_semester_numbers() {
  TEST_ASSERT_EQUAL(1, Semesters::yearOf(1));
  TEST_ASSERT_EQUAL(1, Semesters::yearOf(2));
  TEST_ASSERT_EQUAL(3, Semesters::yearOf(5));    // David: semester 5 = year 3, winter
  TEST_ASSERT_EQUAL(3, Semesters::yearOf(6));
  TEST_ASSERT_EQUAL(6, Semesters::yearOf(12));
  TEST_ASSERT_EQUAL(Subjects::kWinter, Semesters::halfOf(5));
  TEST_ASSERT_EQUAL(Subjects::kSummer, Semesters::halfOf(6));
  TEST_ASSERT_EQUAL(Subjects::kWinter, Semesters::halfOf(1));
  TEST_ASSERT_EQUAL(Subjects::kSummer, Semesters::halfOf(12));
}

void test_semester_number_from_year_and_half() {
  TEST_ASSERT_EQUAL(5, Semesters::number(3, Subjects::kWinter));
  TEST_ASSERT_EQUAL(6, Semesters::number(3, Subjects::kSummer));
  for (uint8_t n = Semesters::kFirst; n <= Semesters::kLast; n++) {
    TEST_ASSERT_EQUAL(n, Semesters::number(Semesters::yearOf(n), Semesters::halfOf(n)));
  }
}

// ---------------------------------------------------------------- subject list
static size_t buildMain(View view, size_t* out) { return SubjectList::buildMain(view, out, 32); }

void test_main_list_filters_by_year_and_semester() {
  size_t list[32];
  size_t n = buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(3, n);
  TEST_ASSERT_EQUAL(kA1, list[0]);   // no study time yet: curriculum order
  TEST_ASSERT_EQUAL(kA2, list[1]);
  TEST_ASSERT_EQUAL(kW1, list[2]);

  n = buildMain({1, Subjects::kSummer}, list);
  TEST_ASSERT_EQUAL(3, n);
  TEST_ASSERT_EQUAL(kS1, list[2]);   // summer subject instead of the winter one

  n = buildMain({2, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(1, n);
  TEST_ASSERT_EQUAL(kB1, list[0]);   // a subject spanning both semesters shows in either

  TEST_ASSERT_EQUAL(0, buildMain({3, Subjects::kWinter}, list));
}

void test_most_studied_comes_first() {
  Progress::recordSession(kA2, 600);
  Progress::recordSession(kW1, 1800);
  size_t list[32];
  size_t n = buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(3, n);
  TEST_ASSERT_EQUAL(kW1, list[0]);
  TEST_ASSERT_EQUAL(kA2, list[1]);
  TEST_ASSERT_EQUAL(kA1, list[2]);
}

void test_equal_time_keeps_curriculum_order() {
  Progress::recordSession(kW1, 600);
  Progress::recordSession(kA2, 600);
  size_t list[32];
  buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(kA2, list[0]);   // same time: lower position wins
  TEST_ASSERT_EQUAL(kW1, list[1]);
}

void test_favorites_go_first_then_by_time() {
  Progress::recordSession(kA1, 5000);   // most studied, not a favourite
  Progress::setFavorite(kW1, true);     // favourite, never studied
  Progress::setFavorite(kA2, true);     // favourite with some time
  Progress::recordSession(kA2, 100);
  size_t list[32];
  buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(kA2, list[0]);   // favourites first, more time first among them
  TEST_ASSERT_EQUAL(kW1, list[1]);
  TEST_ASSERT_EQUAL(kA1, list[2]);   // then the rest
}

void test_elective_joins_main_list_only_with_study_time() {
  size_t list[32];
  TEST_ASSERT_EQUAL(3, buildMain({1, Subjects::kWinter}, list));   // no elective yet

  Progress::recordSession(kE2, 300);
  size_t n = buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(4, n);
  TEST_ASSERT_EQUAL(kE2, list[0]);   // most studied, so it sorts to the top
}

void test_favorite_elective_moves_to_main_list() {
  size_t list[32];
  TEST_ASSERT_EQUAL(3, buildMain({1, Subjects::kWinter}, list));
  Progress::setFavorite(kE1, true);                // never studied, but a favourite
  size_t n = buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(4, n);
  TEST_ASSERT_EQUAL(kE1, list[0]);                 // favourites first
  Progress::setFavorite(kE1, false);
  TEST_ASSERT_EQUAL(3, buildMain({1, Subjects::kWinter}, list));   // back in the folder only
  size_t folder[32];
  TEST_ASSERT_EQUAL(2, SubjectList::buildElectives({1, Subjects::kWinter}, folder, 32));
}

void test_elective_is_tied_to_its_period() {
  size_t list[32];
  Progress::recordSession(kE1, 300);   // E1 is offered in year 1, winter only
  TEST_ASSERT_EQUAL(4, buildMain({1, Subjects::kWinter}, list));
  TEST_ASSERT_EQUAL(3, buildMain({1, Subjects::kSummer}, list));   // not in the summer list
  TEST_ASSERT_EQUAL(0, buildMain({3, Subjects::kWinter}, list));   // nor in other years

  Progress::recordSession(kE2, 300);   // E2 spans both semesters of year 1
  TEST_ASSERT_EQUAL(4, buildMain({1, Subjects::kSummer}, list));
}

void test_elective_folder_lists_all_electives_of_the_period() {
  size_t list[32];
  size_t n = SubjectList::buildElectives({1, Subjects::kWinter}, list, 32);
  TEST_ASSERT_EQUAL(2, n);
  TEST_ASSERT_EQUAL(kE1, list[0]);
  TEST_ASSERT_EQUAL(kE2, list[1]);

  n = SubjectList::buildElectives({1, Subjects::kSummer}, list, 32);
  TEST_ASSERT_EQUAL(1, n);              // only the one spanning both semesters
  TEST_ASSERT_EQUAL(kE2, list[0]);

  n = SubjectList::buildElectives({2, Subjects::kWinter}, list, 32);
  TEST_ASSERT_EQUAL(1, n);
  TEST_ASSERT_EQUAL(kE3, list[0]);
  TEST_ASSERT_EQUAL(0, SubjectList::buildElectives({4, Subjects::kSummer}, list, 32));

  Progress::setFavorite(kE2, true);     // same order rule as the main list
  SubjectList::buildElectives({1, Subjects::kWinter}, list, 32);
  TEST_ASSERT_EQUAL(kE2, list[0]);
}

void test_subject_spanning_two_semesters_shares_one_total() {
  size_t list[32];
  Progress::recordSession(kA1, 600);   // studied while browsing the winter list
  Progress::recordSession(kA1, 900);   // ... and later in the summer list
  TEST_ASSERT_EQUAL(1500, Progress::stats(kA1).totalSeconds);
  TEST_ASSERT_EQUAL(2, Progress::stats(kA1).sessions);
  buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(kA1, list[0]);
  buildMain({1, Subjects::kSummer}, list);
  TEST_ASSERT_EQUAL(kA1, list[0]);     // same subject, same record, in both lists
}

void test_find_returns_minus_one_when_missing() {
  size_t list[32];
  size_t n = buildMain({1, Subjects::kWinter}, list);
  TEST_ASSERT_EQUAL(-1, SubjectList::find(list, n, kB1));
  TEST_ASSERT_EQUAL(1, SubjectList::find(list, n, kA2));
}

// ---------------------------------------------------------------- progress
void test_progress_records_time_and_sessions() {
  Progress::recordSession(kA1, 1500);
  Progress::recordSession(kA1, 900);
  TEST_ASSERT_EQUAL(2400, Progress::stats(kA1).totalSeconds);
  TEST_ASSERT_EQUAL(2, Progress::stats(kA1).sessions);
  TEST_ASSERT_EQUAL(0, Progress::stats(kA2).totalSeconds);
}

void test_progress_favorite_toggle() {
  TEST_ASSERT_TRUE(Progress::toggleFavorite(kA1));
  TEST_ASSERT_TRUE(Progress::stats(kA1).favorite);
  TEST_ASSERT_FALSE(Progress::toggleFavorite(kA1));
}

void test_progress_last_subject_is_remembered_by_id() {
  TEST_ASSERT_EQUAL(-1, Progress::lastSubjectIndex());
  Progress::setLastSubject(kB1);
  TEST_ASSERT_EQUAL(kB1, Progress::lastSubjectIndex());
}

void test_progress_semester_defaults_to_5_and_is_clamped() {
  TEST_ASSERT_EQUAL(5, Progress::currentSemester());
  Progress::setCurrentSemester(0);
  TEST_ASSERT_EQUAL(1, Progress::currentSemester());
  Progress::setCurrentSemester(13);
  TEST_ASSERT_EQUAL(12, Progress::currentSemester());
  Progress::setCurrentSemester(6);
  TEST_ASSERT_EQUAL(6, Progress::currentSemester());
}

void test_progress_target_minutes_is_clamped() {
  TEST_ASSERT_EQUAL(25, Progress::targetMinutes());
  Progress::setTargetMinutes(1);
  TEST_ASSERT_EQUAL(5, Progress::targetMinutes());
  Progress::setTargetMinutes(500);
  TEST_ASSERT_EQUAL(120, Progress::targetMinutes());
  Progress::setTargetMinutes(45);
  TEST_ASSERT_EQUAL(45, Progress::targetMinutes());
}

void test_progress_sounds_default_on_and_can_be_switched() {
  TEST_ASSERT_TRUE(Progress::soundTurn());
  TEST_ASSERT_TRUE(Progress::soundPress());
  TEST_ASSERT_TRUE(Progress::soundNudge());
  Progress::setSoundTurn(false);
  TEST_ASSERT_FALSE(Progress::soundTurn());
  TEST_ASSERT_TRUE(Progress::soundPress());   // independent switches
  Progress::setSoundNudge(false);
  TEST_ASSERT_FALSE(Progress::soundNudge());
  Progress::reset();
  TEST_ASSERT_TRUE(Progress::soundTurn());
  TEST_ASSERT_TRUE(Progress::soundNudge());
}

// ---------------------------------------------------------------- session
void test_session_counts_up_and_nudges_once_but_keeps_running() {
  Session s;
  TEST_ASSERT_EQUAL(Session::Phase::Idle, s.phase());
  s.startStudy(1000, kA1, 1500);
  TEST_ASSERT_EQUAL(Session::Phase::Studying, s.phase());

  TEST_ASSERT_EQUAL(0, s.elapsedSeconds(1000));
  TEST_ASSERT_EQUAL(1499, s.elapsedSeconds(2499));
  TEST_ASSERT_FALSE(s.nudgeDue(2499));

  TEST_ASSERT_TRUE(s.nudgeDue(2500));     // target reached
  TEST_ASSERT_FALSE(s.nudgeDue(2501));    // only once
  TEST_ASSERT_FALSE(s.nudgeDue(9000));

  // Nothing stops the session: it is still studying and still counting.
  TEST_ASSERT_EQUAL(Session::Phase::Studying, s.phase());
  TEST_ASSERT_EQUAL(8000, s.elapsedSeconds(9000));
  TEST_ASSERT_TRUE(s.targetReached(9000));
}

void test_session_pause_stops_counting_and_resume_continues() {
  Session s;
  s.startStudy(1000, kA1, 1500);
  s.pause(1600);                                  // 600 s studied
  TEST_ASSERT_TRUE(s.isPaused());
  TEST_ASSERT_EQUAL(600, s.elapsedSeconds(1600));
  TEST_ASSERT_EQUAL(600, s.elapsedSeconds(9000));  // time passes, the counter does not
  s.resume(9000);
  TEST_ASSERT_FALSE(s.isPaused());
  TEST_ASSERT_EQUAL(600, s.elapsedSeconds(9000));
  TEST_ASSERT_EQUAL(700, s.elapsedSeconds(9100));
}

void test_session_pause_twice_and_resume_when_running_do_nothing() {
  Session s;
  s.startStudy(1000, kA1, 1500);
  s.resume(1100);                                 // not paused: ignored
  TEST_ASSERT_EQUAL(200, s.elapsedSeconds(1200));
  s.pause(1200);
  s.pause(5000);                                  // already paused: must not add the gap
  TEST_ASSERT_EQUAL(200, s.elapsedSeconds(5000));
}

void test_session_nudge_waits_for_counted_time_not_paused_time() {
  Session s;
  s.startStudy(0, kA1, 1500);
  s.pause(1000);
  TEST_ASSERT_FALSE(s.nudgeDue(100000));          // a long pause never reaches the target
  s.resume(100000);
  TEST_ASSERT_FALSE(s.nudgeDue(100499));
  TEST_ASSERT_TRUE(s.nudgeDue(100500));           // 1000 + 500 counted seconds
}

void test_session_can_be_stopped_while_paused() {
  Session s;
  s.startStudy(1000, kA1, 1500);
  s.pause(1300);
  uint32_t length = s.endStudy(7000);
  TEST_ASSERT_EQUAL(300, length);
  TEST_ASSERT_EQUAL(Session::Phase::Break, s.phase());
  TEST_ASSERT_FALSE(s.isPaused());
  TEST_ASSERT_EQUAL(10, s.elapsedSeconds(7010));  // the break counts normally
}

void test_session_target_can_change_while_running() {
  Session s;
  s.startStudy(0, kA1, 1500);
  s.setTargetSeconds(600);
  TEST_ASSERT_TRUE(s.nudgeDue(600));
}

void test_session_end_starts_break() {
  Session s;
  s.startStudy(1000, kA1, 1500);
  uint32_t length = s.endStudy(1000 + 1800);
  TEST_ASSERT_EQUAL(1800, length);
  TEST_ASSERT_EQUAL(Session::Phase::Break, s.phase());
  TEST_ASSERT_EQUAL(0, s.elapsedSeconds(2800));    // the break starts counting from zero
  TEST_ASSERT_EQUAL(120, s.elapsedSeconds(2920));
  TEST_ASSERT_EQUAL(kA1, s.subjectIndex());        // a new block can reuse the subject
  s.endBreak();
  TEST_ASSERT_EQUAL(Session::Phase::Idle, s.phase());
}

void test_session_clock_going_backwards_does_not_wrap() {
  Session s;
  s.startStudy(5000, kA1, 1500);
  TEST_ASSERT_EQUAL(0, s.elapsedSeconds(4000));
  TEST_ASSERT_FALSE(s.nudgeDue(4000));
}

void test_session_short_blocks_are_not_counted() {
  TEST_ASSERT_FALSE(Session::isCounted(59));
  TEST_ASSERT_TRUE(Session::isCounted(60));
}

// ---------------------------------------------------------------- date and time editing
using DateEdit::Field;
using DateEdit::Fields;

void test_days_in_month_and_leap_years() {
  TEST_ASSERT_EQUAL(31, DateEdit::daysInMonth(2026, 1));
  TEST_ASSERT_EQUAL(30, DateEdit::daysInMonth(2026, 11));
  TEST_ASSERT_EQUAL(28, DateEdit::daysInMonth(2026, 2));
  TEST_ASSERT_EQUAL(29, DateEdit::daysInMonth(2028, 2));
  TEST_ASSERT_EQUAL(28, DateEdit::daysInMonth(2100, 2));   // divisible by 100: not a leap year
  TEST_ASSERT_EQUAL(29, DateEdit::daysInMonth(2000, 2));   // divisible by 400: leap year
}

void test_date_edit_wraps_day_month_hour_minute() {
  Fields f = {2026, 11, 30, 23, 59};
  DateEdit::change(f, Field::Day, +1);    TEST_ASSERT_EQUAL(1, f.day);
  DateEdit::change(f, Field::Day, -1);    TEST_ASSERT_EQUAL(30, f.day);
  DateEdit::change(f, Field::Month, +2);  TEST_ASSERT_EQUAL(1, f.month);   // 11 -> 12 -> 1
  DateEdit::change(f, Field::Hour, +1);   TEST_ASSERT_EQUAL(0, f.hour);
  DateEdit::change(f, Field::Hour, -1);   TEST_ASSERT_EQUAL(23, f.hour);
  DateEdit::change(f, Field::Minute, +1); TEST_ASSERT_EQUAL(0, f.minute);
  DateEdit::change(f, Field::Minute, -1); TEST_ASSERT_EQUAL(59, f.minute);
  TEST_ASSERT_EQUAL(23, f.hour);          // minutes wrapping never touches the hour
}

void test_date_edit_keeps_day_valid_when_month_gets_shorter() {
  Fields f = {2026, 1, 31, 12, 0};
  DateEdit::change(f, Field::Month, +1);  // 31 January -> February
  TEST_ASSERT_EQUAL(2, f.month);
  TEST_ASSERT_EQUAL(28, f.day);

  Fields g = {2028, 3, 31, 12, 0};
  DateEdit::change(g, Field::Month, -1);  // -> February of a leap year
  TEST_ASSERT_EQUAL(29, g.day);

  Fields h = {2028, 2, 29, 12, 0};
  DateEdit::change(h, Field::Year, +1);   // 29 Feb 2029 does not exist
  TEST_ASSERT_EQUAL(28, h.day);
}

void test_date_edit_year_stops_at_limits() {
  Fields f = {DateEdit::kMinYear, 6, 1, 0, 0};
  DateEdit::change(f, Field::Year, -1);
  TEST_ASSERT_EQUAL(DateEdit::kMinYear, f.year);
  f.year = DateEdit::kMaxYear;
  DateEdit::change(f, Field::Year, +1);
  TEST_ASSERT_EQUAL(DateEdit::kMaxYear, f.year);
}

// ---------------------------------------------------------------- formatting
void test_format_elapsed() {
  char buf[16];
  Format::elapsed(buf, sizeof buf, 0);     TEST_ASSERT_EQUAL_STRING("00:00", buf);
  Format::elapsed(buf, sizeof buf, 65);    TEST_ASSERT_EQUAL_STRING("01:05", buf);
  Format::elapsed(buf, sizeof buf, 3599);  TEST_ASSERT_EQUAL_STRING("59:59", buf);
  Format::elapsed(buf, sizeof buf, 3723);  TEST_ASSERT_EQUAL_STRING("1:02:03", buf);
}

void test_format_duration() {
  char buf[24];
  Format::duration(buf, sizeof buf, 0);      TEST_ASSERT_EQUAL_STRING("0 min", buf);
  Format::duration(buf, sizeof buf, 2759);   TEST_ASSERT_EQUAL_STRING("45 min", buf);
  Format::duration(buf, sizeof buf, 12300);  TEST_ASSERT_EQUAL_STRING("3 h 25 min", buf);
  Format::duration(buf, sizeof buf, 43200);  TEST_ASSERT_EQUAL_STRING("12 h", buf);
}

void test_format_date_and_clock_and_period() {
  char buf[40];
  Format::date(buf, sizeof buf, 4, 5, 11, 2026);
  TEST_ASSERT_EQUAL_STRING("četvrtak, 5. 11. 2026.", buf);
  Format::clock(buf, sizeof buf, 7, 5, 9);
  TEST_ASSERT_EQUAL_STRING("07:05:09", buf);
  Format::period(buf, sizeof buf, 3, Subjects::kWinter);
  TEST_ASSERT_EQUAL_STRING("3. godina, zimski", buf);
  Format::period(buf, sizeof buf, 3, Subjects::kSummer);
  TEST_ASSERT_EQUAL_STRING("3. godina, ljetni", buf);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_semester_numbers);
  RUN_TEST(test_semester_number_from_year_and_half);
  RUN_TEST(test_main_list_filters_by_year_and_semester);
  RUN_TEST(test_most_studied_comes_first);
  RUN_TEST(test_equal_time_keeps_curriculum_order);
  RUN_TEST(test_favorites_go_first_then_by_time);
  RUN_TEST(test_elective_joins_main_list_only_with_study_time);
  RUN_TEST(test_favorite_elective_moves_to_main_list);
  RUN_TEST(test_elective_is_tied_to_its_period);
  RUN_TEST(test_elective_folder_lists_all_electives_of_the_period);
  RUN_TEST(test_subject_spanning_two_semesters_shares_one_total);
  RUN_TEST(test_find_returns_minus_one_when_missing);
  RUN_TEST(test_progress_records_time_and_sessions);
  RUN_TEST(test_progress_favorite_toggle);
  RUN_TEST(test_progress_last_subject_is_remembered_by_id);
  RUN_TEST(test_progress_semester_defaults_to_5_and_is_clamped);
  RUN_TEST(test_progress_target_minutes_is_clamped);
  RUN_TEST(test_progress_sounds_default_on_and_can_be_switched);
  RUN_TEST(test_session_counts_up_and_nudges_once_but_keeps_running);
  RUN_TEST(test_session_pause_stops_counting_and_resume_continues);
  RUN_TEST(test_session_pause_twice_and_resume_when_running_do_nothing);
  RUN_TEST(test_session_nudge_waits_for_counted_time_not_paused_time);
  RUN_TEST(test_session_can_be_stopped_while_paused);
  RUN_TEST(test_session_target_can_change_while_running);
  RUN_TEST(test_session_end_starts_break);
  RUN_TEST(test_session_clock_going_backwards_does_not_wrap);
  RUN_TEST(test_session_short_blocks_are_not_counted);
  RUN_TEST(test_days_in_month_and_leap_years);
  RUN_TEST(test_date_edit_wraps_day_month_hour_minute);
  RUN_TEST(test_date_edit_keeps_day_valid_when_month_gets_shorter);
  RUN_TEST(test_date_edit_year_stops_at_limits);
  RUN_TEST(test_format_elapsed);
  RUN_TEST(test_format_duration);
  RUN_TEST(test_format_date_and_clock_and_period);
  return UNITY_END();
}
