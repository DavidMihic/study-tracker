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
