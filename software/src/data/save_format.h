// The byte layout of everything that is stored in flash, and the code that writes and reads it.
// Plain C++ (no Arduino), so the PC tests can check it, including damaged data.
//
// Layout (all numbers little-endian):
//   "STUD"  version(1)  reserved(1)  payloadLength(2)   <- header, 8 bytes
//   payload (see save_format.cpp)
//   CRC-32 of header + payload (4)
// Anything that does not match (wrong marker/version/length/CRC) is rejected as a whole, so the firmware
// then starts from defaults instead of trusting damaged data.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "data/subjects.h"

namespace SaveFormat {

// Version 2 added ActiveRecord::startedUnix. Version 1 data is still read (startedUnix then falls back to
// runStartUnix); everything is written as version 2.
constexpr uint8_t kVersion = 2;

struct SubjectRecord {
  uint16_t id;
  uint32_t totalSeconds;
  uint16_t sessions;
  bool favorite;
};

// The study block that was running when the data was written (if any).
struct ActiveRecord {
  bool active;
  uint16_t subjectId;
  uint32_t accumulatedSeconds;   // counted before the running stretch started
  uint32_t runStartUnix;         // start of the running stretch
  bool running;                  // false = it was paused
  uint32_t checkpointUnix;       // last moment the device is known to have been running
  uint32_t startedUnix;          // when the block was first started (for the session log)
};

struct Data {
  uint8_t semester;
  uint16_t targetMinutes;
  uint8_t brightness;
  bool soundTurn, soundPress, soundNudge;
  bool soundRestore;             // stored as "alert switched OFF" (bit 3), so data from before this switch existed reads as ON
  uint16_t lastSubjectId;        // 0 = none
  ActiveRecord active;
  uint16_t subjectCount;
  SubjectRecord subjects[Subjects::kMaxSubjects];
};

// Largest possible serialized size (all subjects present).
constexpr size_t kMaxBytes = 8 + 1 + 2 + 1 + 1 + 2 + (1 + 2 + 4 + 4 + 1 + 4 + 4) + 2 +
                             Subjects::kMaxSubjects * (2 + 4 + 2 + 1) + 4;

// Writes data into out (capacity bytes). Returns the length, or 0 if it does not fit.
size_t serialize(const Data& data, uint8_t* out, size_t capacity);

// Reads bytes written by serialize(). Returns false (and leaves data unspecified) if they are not valid.
bool parse(const uint8_t* bytes, size_t length, Data& data);

uint32_t crc32(const uint8_t* bytes, size_t length);

}  // namespace SaveFormat
