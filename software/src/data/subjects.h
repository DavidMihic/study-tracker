// The hard-coded subject list. This is the file David edits to enter the real curriculum
// (the list itself is in subjects.cpp). Plain C++ (no Arduino), so the PC tests can use it.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Subjects {

// Which semesters a subject is taught in (bit flags). A subject that spans the whole year has both.
enum Semester : uint8_t {
  kWinter = 1,
  kSummer = 2,
  kBoth   = kWinter | kSummer,
};

struct Subject {
  uint16_t id;          // STABLE number, saved in flash. Never change or reuse an id; only add new ones.
  const char* name;     // shown on screen, UTF-8, Croatian
  uint8_t year;         // study year 1..6 (electives too: they are offered in a specific year)
  uint8_t semesters;    // Semester flags; a subject spanning both has one id and ONE set of totals
  bool elective;        // electives live in the folder of their period until they have study time
};

// Upper bound used for the fixed-size tables in progress.cpp. A compile-time check in subjects.cpp
// fails the build if the list outgrows it.
constexpr size_t kMaxSubjects = 160;

size_t count();
const Subject& at(size_t index);

// Position in the list for a saved id, or -1 if no such subject exists (e.g. removed from the list).
int indexOfId(uint16_t id);

}  // namespace Subjects
