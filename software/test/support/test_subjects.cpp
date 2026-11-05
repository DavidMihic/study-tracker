// Fixed subject list used ONLY by the PC tests (it replaces data/subjects.cpp in the native env),
// so the tests keep working when David edits the real curriculum.
#include "data/subjects.h"

namespace Subjects {

namespace {

const Subject kAll[] = {
  /* 0 */ {1, "A1", 1, kBoth,   false},
  /* 1 */ {2, "A2", 1, kBoth,   false},
  /* 2 */ {3, "W1", 1, kWinter, false},
  /* 3 */ {4, "S1", 1, kSummer, false},
  /* 4 */ {5, "B1", 2, kBoth,   false},
  /* 5 */ {6, "E1", 1, kWinter, true},
  /* 6 */ {7, "E2", 1, kBoth,   true},
  /* 7 */ {8, "E3", 2, kWinter, true},
};

constexpr size_t kAllCount = sizeof(kAll) / sizeof(kAll[0]);

}  // namespace

size_t count() { return kAllCount; }
const Subject& at(size_t index) { return kAll[index]; }

int indexOfId(uint16_t id) {
  for (size_t i = 0; i < kAllCount; i++) {
    if (kAll[i].id == id) return (int)i;
  }
  return -1;
}

}  // namespace Subjects
