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

// "petak, 5. 11. 2026." weekday: 0 = Sunday ... 6 = Saturday.
void date(char* out, size_t size, uint8_t weekday, int day, int month, int year);

// "HH:MM:SS" for the home screen (digits and ':' only).
void clock(char* out, size_t size, int hour, int minute, int second);

// "3. godina, zimski" for the period shown in the picker. semester: Subjects::kWinter or kSummer.
void period(char* out, size_t size, uint8_t year, uint8_t semester);

}  // namespace Format
