// Builds the ordered subject lists for the picker. Plain C++ (no Arduino), so the PC tests can use it.
//
// Order rule (David): favourites first, then the subjects she studies most (total time, longest first),
// and subjects with equal time keep their order from the curriculum list.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace SubjectList {

// A year and one semester (Subjects::kWinter or Subjects::kSummer): what the main list is showing.
struct View {
  uint8_t year;
  uint8_t semester;
};

// Main list: the subjects of that year and semester. An elective of that period is in it only once it
// has study time. Writes subject positions (indexes into the Subjects list) into out and returns how many.
size_t buildMain(View view, size_t* out, size_t maxEntries);

// The elective folder of that period: ALL its electives (studied or not), same order rule.
size_t buildElectives(View view, size_t* out, size_t maxEntries);

// Position of subjectIndex inside a built list, or -1 if it is not in it.
int find(const size_t* list, size_t count, size_t subjectIndex);

}  // namespace SubjectList
