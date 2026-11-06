// Turns numbers into the text shown on screen. Plain C++ (no Arduino), so the PC tests can use it.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Format {

// Running clock for the timers: "05:03" under one hour, "1:05:03" from one hour on.
// Only digits and ':' (the big clock font has no other characters).
void elapsed(char* out, size_t size, uint32_t seconds);

// Total study time for lists: "0 min", "45 min", "3 h 25 min", "12 h". Seconds are cut off, not rounded.
void duration(char* out, size_t size, uint32_t seconds);

// Short label for a bar of the week chart: "47min" under one hour, otherwise rounded to the nearest
// half hour: "1h", "1,5h", "2h" (decimal comma, as the display is Croatian).
void barLabel(char* out, size_t size, uint32_t seconds);

// "1 blok", "3 bloka", "12 blokova" (Croatian plural).
void blocks(char* out, size_t size, uint32_t count);

// Calendar date of a day number (whole days since 1970-01-01): "5. 11." and "2. 11. - 8. 11.".
void dayMonth(char* out, size_t size, uint32_t dayNumber);
void dayRange(char* out, size_t size, uint32_t firstDay, uint32_t lastDay);

// "petak, 5. 11. 2026." weekday: 0 = Sunday ... 6 = Saturday.
void date(char* out, size_t size, uint8_t weekday, int day, int month, int year);

// "HH:MM:SS" for the home screen (digits and ':' only).
void clock(char* out, size_t size, int hour, int minute, int second);

// "3. godina, zimski" for the period shown in the picker. semester: Subjects::kWinter or kSummer.
void period(char* out, size_t size, uint8_t year, uint8_t semester);

}  // namespace Format
