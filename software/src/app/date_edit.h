// Logic for setting the date and time with the dial: which values exist, how they wrap and that the day
// always fits the month. Plain C++ (no Arduino), so the PC tests can use it.
#pragma once
#include <stdint.h>

namespace DateEdit {

struct Fields {
  uint16_t year;
  uint8_t month;    // 1..12
  uint8_t day;      // 1..days in that month
  uint8_t hour;     // 0..23
  uint8_t minute;   // 0..59
};

enum class Field : uint8_t { Day, Month, Year, Hour, Minute };

constexpr uint16_t kMinYear = 2024;
constexpr uint16_t kMaxYear = 2099;

uint8_t daysInMonth(uint16_t year, uint8_t month);

// Moves one field by delta steps (+1 / -1). Day, month, hour and minute wrap around (23:59 + 1 minute
// = 23:00, the hour stays); the year stops at the limits. Afterwards the day is cut back if the month
// became shorter (31 Jan -> Feb gives 28 or 29).
void change(Fields& fields, Field field, int delta);

}  // namespace DateEdit
