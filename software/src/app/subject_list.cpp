#include "app/subject_list.h"
#include "data/progress.h"
#include "data/subjects.h"

namespace {

// True if subject a must come before subject b.
bool comesBefore(size_t a, size_t b) {
  const SubjectStats& sa = Progress::stats(a);
  const SubjectStats& sb = Progress::stats(b);
  if (sa.favorite != sb.favorite) return sa.favorite;                     // favourites first
  if (sa.totalSeconds != sb.totalSeconds) return sa.totalSeconds > sb.totalSeconds;   // most studied first
  return a < b;                                                           // curriculum order
}

bool inPeriod(const Subjects::Subject& s, SubjectList::View view) {
  return s.year == view.year && (s.semesters & view.semester) != 0;
}

// Insertion sort: the lists have a few dozen entries at most, and it is easy to read.
void sortList(size_t* list, size_t count) {
  for (size_t i = 1; i < count; i++) {
    size_t value = list[i];
    size_t j = i;
    while (j > 0 && comesBefore(value, list[j - 1])) {
      list[j] = list[j - 1];
      j--;
    }
    list[j] = value;
  }
}

}  // namespace

namespace SubjectList {

size_t buildMain(View view, size_t* out, size_t maxEntries) {
  size_t n = 0;
  for (size_t i = 0; i < Subjects::count() && n < maxEntries; i++) {
    const Subjects::Subject& s = Subjects::at(i);
    if (!inPeriod(s, view)) continue;
    // An elective is in the main list once it was studied or made a favourite; before that it is
    // only in the folder.
    if (s.elective && Progress::stats(i).totalSeconds == 0 && !Progress::stats(i).favorite) continue;
    out[n++] = i;
  }
  sortList(out, n);
  return n;
}

size_t buildElectives(View view, size_t* out, size_t maxEntries) {
  size_t n = 0;
  for (size_t i = 0; i < Subjects::count() && n < maxEntries; i++) {
    const Subjects::Subject& s = Subjects::at(i);
    if (s.elective && inPeriod(s, view)) out[n++] = i;
  }
  sortList(out, n);
  return n;
}

size_t buildStudied(size_t* out, size_t maxEntries) {
  size_t n = 0;
  for (size_t i = 0; i < Subjects::count() && n < maxEntries; i++) {
    if (Progress::hasStats(i)) out[n++] = i;
  }
  sortList(out, n);
  return n;
}

int find(const size_t* list, size_t count, size_t subjectIndex) {
  for (size_t i = 0; i < count; i++) {
    if (list[i] == subjectIndex) return (int)i;
  }
  return -1;
}

}  // namespace SubjectList
