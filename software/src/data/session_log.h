// The session log: one small record for every study block she finishes, kept for the day/week statistics.
// Append-only (a record is never changed), so a power cut can at most damage the record being written,
// and the damaged one is recognised by its checksum and skipped. Plain C++ (no Arduino), so the PC tests
// can check it; the bytes live behind the Backend interface (a LittleFS file on the device).
//
// Record (12 bytes, little-endian): startUnix(4) countedSeconds(4) subjectId(2) crc(2).
// startUnix is when the block was first started; countedSeconds excludes pauses. The block belongs to
// the study day (see studyDay) in which it started, whatever the pauses and the end time were.
// About 12 bytes x 10 blocks x 365 days = 44 KB a year.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace SessionLog {

constexpr size_t kRecordBytes = 12;

struct Record {
  uint32_t startUnix;
  uint32_t seconds;
  uint16_t subjectId;
};

// Where the bytes live. replace() must swap the whole content in one step (the device writes a new
// file and renames it), so a power cut leaves either the old or the new content.
class Backend {
 public:
  virtual ~Backend() = default;
  virtual size_t size() = 0;
  virtual size_t read(size_t offset, uint8_t* buffer, size_t length) = 0;
  virtual bool append(const uint8_t* bytes, size_t length) = 0;
  virtual bool replace(const uint8_t* bytes, size_t length) = 0;
};

// Call once at start. Cuts off a half-written record at the end (left by a power cut), so the next
// append is aligned again.
void begin(Backend* backend);

bool append(const Record& record);

// Number of valid records.
size_t count();

// Calls fn for every valid record, oldest first. Damaged records are skipped.
void forEach(void (*fn)(const Record& record, void* context), void* context);

// Deletes the records of one subject / of everything (the counters were reset).
bool removeSubject(uint16_t subjectId);
bool clear();

// Study day number of a unix time: whole days since 1970-01-01, where a day starts at
// StudyRules::kStudyDayStartHour. Consecutive days have consecutive numbers.
uint32_t studyDay(uint32_t unix);

// 0 = Sunday ... 6 = Saturday, matching Strings::kWeekdays order.
uint8_t weekdayOfStudyDay(uint32_t studyDay);

}  // namespace SessionLog
