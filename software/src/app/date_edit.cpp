#include "app/date_edit.h"

namespace DateEdit {

namespace {

// Wraps value+delta into first..last.
int wrap(int value, int delta, int first, int last) {
  int range = last - first + 1;
  int v = (value - first + delta) % range;
  if (v < 0) v += range;
  return first + v;
}

}  // namespace

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 31;
  bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  return (month == 2 && leap) ? 29 : days[month - 1];
}

void change(Fields& f, Field field, int delta) {
  switch (field) {
    case Field::Day:    f.day = (uint8_t)wrap(f.day, delta, 1, daysInMonth(f.year, f.month)); break;
    case Field::Month:  f.month = (uint8_t)wrap(f.month, delta, 1, 12); break;
    case Field::Year: {
      int y = (int)f.year + delta;
      if (y < kMinYear) y = kMinYear;
      if (y > kMaxYear) y = kMaxYear;
      f.year = (uint16_t)y;
      break;
    }
    case Field::Hour:   f.hour = (uint8_t)wrap(f.hour, delta, 0, 23); break;
    case Field::Minute: f.minute = (uint8_t)wrap(f.minute, delta, 0, 59); break;
  }
  uint8_t last = daysInMonth(f.year, f.month);
  if (f.day > last) f.day = last;
}

}  // namespace DateEdit
