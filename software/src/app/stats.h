// Statistics computed from the session log: time per study day, per week and per subject. Plain C++
// (no Arduino), so the PC tests can check it. Days are study-day numbers (SessionLog::studyDay, a day
// starts at 05:00); weeks run Monday to Sunday. Each call reads the log once.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Stats {

struct Totals {
  uint32_t seconds;
  uint32_t sessions;
};

// An inclusive range of study days.
struct DayRange {
  uint32_t firstDay;
  uint32_t lastDay;
};

struct SubjectTotal {
  uint16_t subjectId;
  uint32_t seconds;
  uint32_t sessions;
};

// The Monday of the week that contains this study day, and the Sunday.
uint32_t weekStart(uint32_t studyDay);
inline uint32_t weekEnd(uint32_t studyDay) { return weekStart(studyDay) + 6; }
inline DayRange weekOf(uint32_t studyDay) { return {weekStart(studyDay), weekEnd(studyDay)}; }

// Everything studied in the range.
Totals total(DayRange range);

// Fills out[0..days) with the totals of firstDay, firstDay + 1, ... (a day without study has zeros).
void perDay(uint32_t firstDay, size_t days, Totals* out);

// Per-subject totals in the range, most time first (ties: lower id first). Subjects with no time in the
// range are left out. Returns how many were written to out (at most capacity).
size_t perSubject(DayRange range, SubjectTotal* out, size_t capacity);

}  // namespace Stats
