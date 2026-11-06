#include "ui/format.h"
#include <stdio.h>
#include "data/subjects.h"
#include "ui/strings.h"

namespace Format {

void elapsed(char* out, size_t size, uint32_t seconds) {
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds / 60) % 60;
  uint32_t secs = seconds % 60;
  if (hours > 0) {
    snprintf(out, size, "%lu:%02lu:%02lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)secs);
  } else {
    snprintf(out, size, "%02lu:%02lu", (unsigned long)minutes, (unsigned long)secs);
  }
}

void duration(char* out, size_t size, uint32_t seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds / 60) % 60;
  if (hours == 0) {
    snprintf(out, size, "%lu %s", minutes, Strings::kMinutes);
  } else if (minutes == 0) {
    snprintf(out, size, "%lu %s", hours, Strings::kHours);
  } else {
    snprintf(out, size, "%lu %s %lu %s", hours, Strings::kHours, minutes, Strings::kMinutes);
  }
}

void barLabel(char* out, size_t size, uint32_t seconds) {
  unsigned long minutes = (seconds + 30) / 60;
  if (minutes < 60) {
    snprintf(out, size, "%lu%s", minutes < 1 ? 1UL : minutes, Strings::kMinutes);
    return;
  }
  unsigned long halfHours = (seconds + 900) / 1800;
  if (halfHours % 2 == 0) snprintf(out, size, "%lu%s", halfHours / 2, Strings::kHours);
  else snprintf(out, size, "%lu,5%s", halfHours / 2, Strings::kHours);
}

void blocks(char* out, size_t size, uint32_t count) {
  uint32_t last = count % 10, lastTwo = count % 100;
  const char* word = Strings::kBlockMany;
  if (lastTwo < 11 || lastTwo > 14) {
    if (last == 1) word = Strings::kBlockOne;
    else if (last >= 2 && last <= 4) word = Strings::kBlockFew;
  }
  snprintf(out, size, "%lu %s", (unsigned long)count, word);
}

namespace {

// Day number (days since 1970-01-01) to month and day of month. Standard civil-from-days calculation
// (H. Hinnant), valid for the whole Gregorian calendar; the year is not needed here.
void monthAndDay(uint32_t dayNumber, int& month, int& day) {
  int32_t z = (int32_t)dayNumber + 719468;
  int32_t era = z / 146097;
  int32_t dayOfEra = z - era * 146097;
  int32_t yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
  int32_t dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);   // from 1 March
  int32_t mp = (5 * dayOfYear + 2) / 153;
  day = dayOfYear - (153 * mp + 2) / 5 + 1;
  month = mp < 10 ? mp + 3 : mp - 9;
}

}  // namespace

void dayMonth(char* out, size_t size, uint32_t dayNumber) {
  int month, day;
  monthAndDay(dayNumber, month, day);
  snprintf(out, size, "%d. %d.", day, month);
}

void dayRange(char* out, size_t size, uint32_t firstDay, uint32_t lastDay) {
  int m1, d1, m2, d2;
  monthAndDay(firstDay, m1, d1);
  monthAndDay(lastDay, m2, d2);
  snprintf(out, size, "%d. %d. - %d. %d.", d1, m1, d2, m2);
}

void date(char* out, size_t size, uint8_t weekday, int day, int month, int year) {
  snprintf(out, size, "%s, %d. %d. %d.", Strings::kWeekdays[weekday % 7], day, month, year);
}

void clock(char* out, size_t size, int hour, int minute, int second) {
  snprintf(out, size, "%02d:%02d:%02d", hour, minute, second);
}

void period(char* out, size_t size, uint8_t year, uint8_t semester) {
  const char* name = (semester == Subjects::kSummer) ? Strings::kSummerSemester : Strings::kWinterSemester;
  snprintf(out, size, Strings::kPeriodFormat, (unsigned)year, name);
}

}  // namespace Format
