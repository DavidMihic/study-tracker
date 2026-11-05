#include "app/semester.h"
#include "data/subjects.h"

namespace Semesters {

uint8_t yearOf(uint8_t semester) { return (uint8_t)((semester + 1) / 2); }

uint8_t halfOf(uint8_t semester) { return (semester % 2 == 1) ? Subjects::kWinter : Subjects::kSummer; }

uint8_t number(uint8_t year, uint8_t half) {
  return (uint8_t)(year * 2 - (half == Subjects::kWinter ? 1 : 0));
}

}  // namespace Semesters
