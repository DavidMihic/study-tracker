#include "data/subjects.h"

namespace Subjects {

namespace {

// ===========================================================================================
// PLACEHOLDER DATA. These are example subjects so the firmware can be tested; replace them
// with the real curriculum. Rules for editing:
//  - Give every new subject the next unused id and never change an existing id.
//  - Fields: {id, name, year, semesters, elective}
//  - Electives are entered like other subjects (their year and semester) with elective = true.
// ===========================================================================================
const Subject kAll[] = {
  // 1. godina
  {101, "Anatomija",                  1, kBoth,   false},
  {102, "Histologija i embriologija", 1, kBoth,   false},
  {103, "Medicinska biokemija",       1, kWinter, false},
  {104, "Fiziologija",                1, kSummer, false},
  // 2. godina
  {201, "Patološka fiziologija",      2, kBoth,   false},
  {202, "Mikrobiologija",             2, kSummer, false},
  // 3. godina
  {301, "Interna medicina",           3, kBoth,   false},
  {302, "Farmakologija",              3, kWinter, false},
  {303, "Kirurgija",                  3, kSummer, false},
  // izborni predmeti
  {901, "Medicinska etika",           1, kWinter, true},
  {902, "Prva pomoć",                 2, kSummer, true},
  {903, "Klinička prehrana",          3, kWinter, true},
  {904, "Medicinska komunikacija",    3, kBoth,   true},
  {905, "Sportska medicina",          4, kSummer, true},
};

constexpr size_t kAllCount = sizeof(kAll) / sizeof(kAll[0]);
static_assert(kAllCount <= kMaxSubjects, "Subjects::kMaxSubjects is too small for the subject list");

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
