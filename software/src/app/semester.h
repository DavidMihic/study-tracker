// Semester numbers. Her studies have 12 semesters: 1 = 1st year winter, 2 = 1st year summer, ... 12 = 6th
// year summer. Odd numbers are winter, even are summer. The current semester is chosen by her (the
// school's calendar is unclear), never taken from the date. Plain C++ (no Arduino), so the PC tests can
// use it.
#pragma once
#include <stdint.h>

namespace Semesters {

constexpr uint8_t kFirst = 1;
constexpr uint8_t kLast  = 12;

// Study year 1..6 of a semester number.
uint8_t yearOf(uint8_t semester);

// Subjects::kWinter or Subjects::kSummer.
uint8_t halfOf(uint8_t semester);

// Semester number of a year (1..6) and a half (Subjects::kWinter / kSummer).
uint8_t number(uint8_t year, uint8_t half);

}  // namespace Semesters
