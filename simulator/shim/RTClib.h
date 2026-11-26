// SIMULATOR ONLY. The DateTime class of RTClib (the only part the screens use), built on the C library.
// Times have no time zone: the web page passes in the local wall-clock time as if it were UTC.
#pragma once
#include <stdint.h>
#include <time.h>

class DateTime {
 public:
  DateTime(uint32_t unix = 946684800u) : unix_(unix) { split(); }
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0) {
    struct tm parts = {};
    parts.tm_year = year - 1900;
    parts.tm_mon = month - 1;
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_sec = second;
    unix_ = (uint32_t)timegm(&parts);
    split();
  }

  uint32_t unixtime() const { return unix_; }
  uint16_t year() const { return (uint16_t)(parts_.tm_year + 1900); }
  uint8_t month() const { return (uint8_t)(parts_.tm_mon + 1); }
  uint8_t day() const { return (uint8_t)parts_.tm_mday; }
  uint8_t hour() const { return (uint8_t)parts_.tm_hour; }
  uint8_t minute() const { return (uint8_t)parts_.tm_min; }
  uint8_t second() const { return (uint8_t)parts_.tm_sec; }
  uint8_t dayOfTheWeek() const { return (uint8_t)parts_.tm_wday; }   // 0 = Sunday

 private:
  void split() {
    const time_t value = (time_t)unix_;
    gmtime_r(&value, &parts_);
  }
  uint32_t unix_;
  struct tm parts_;
};
