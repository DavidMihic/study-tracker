#include "data/session_log.h"
#include <string.h>
#include <vector>
#include "config/study_rules.h"
#include "data/save_format.h"   // crc32

namespace SessionLog {

namespace {

Backend* backend = nullptr;

void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
uint32_t get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

uint16_t checksum(const uint8_t* record) { return (uint16_t)SaveFormat::crc32(record, kRecordBytes - 2); }

void encode(const Record& r, uint8_t* out) {
  put32(out, r.startUnix);
  put32(out + 4, r.seconds);
  out[8] = (uint8_t)r.subjectId;
  out[9] = (uint8_t)(r.subjectId >> 8);
  uint16_t crc = checksum(out);
  out[10] = (uint8_t)crc;
  out[11] = (uint8_t)(crc >> 8);
}

bool decode(const uint8_t* in, Record& r) {
  uint16_t crc = (uint16_t)(in[10] | (in[11] << 8));
  if (crc != checksum(in)) return false;
  r.startUnix = get32(in);
  r.seconds = get32(in + 4);
  r.subjectId = (uint16_t)(in[8] | (in[9] << 8));
  return true;
}

// Reads all valid records into memory (the log is small: ~44 KB a year). *complete is false if a read
// failed half-way: what was read is then NOT the whole log, so nobody may write it back over the file.
std::vector<Record> readAll(bool* complete = nullptr) {
  std::vector<Record> records;
  if (complete) *complete = false;
  if (!backend) return records;
  const size_t total = backend->size() / kRecordBytes;
  records.reserve(total);
  uint8_t chunk[kRecordBytes * 32];
  for (size_t done = 0; done < total;) {
    size_t n = total - done < 32 ? total - done : 32;
    if (backend->read(done * kRecordBytes, chunk, n * kRecordBytes) != n * kRecordBytes) return records;
    for (size_t i = 0; i < n; i++) {
      Record r;
      if (decode(chunk + i * kRecordBytes, r)) records.push_back(r);
    }
    done += n;
  }
  if (complete) *complete = true;
  return records;
}

bool writeAll(const std::vector<Record>& records) {
  std::vector<uint8_t> bytes(records.size() * kRecordBytes);
  for (size_t i = 0; i < records.size(); i++) encode(records[i], &bytes[i * kRecordBytes]);
  return backend->replace(bytes.data(), bytes.size());
}

}  // namespace

void begin(Backend* b) {
  backend = b;
  if (backend && backend->size() % kRecordBytes != 0) {   // drop the torn tail
    bool complete = false;
    std::vector<Record> records = readAll(&complete);
    if (complete) writeAll(records);
  }
}

bool append(const Record& record) {
  if (!backend) return false;
  uint8_t bytes[kRecordBytes];
  encode(record, bytes);
  return backend->append(bytes, sizeof bytes);
}

size_t count() { return readAll().size(); }

void forEach(void (*fn)(const Record&, void*), void* context) {
  for (const Record& r : readAll()) fn(r, context);
}

bool removeSubject(uint16_t subjectId) {
  if (!backend) return false;
  bool complete = false;
  std::vector<Record> all = readAll(&complete);
  if (!complete) return false;   // a failed read must not turn into a shortened log
  std::vector<Record> kept;
  for (const Record& r : all) if (r.subjectId != subjectId) kept.push_back(r);
  return writeAll(kept);
}

bool clear() {
  if (!backend) return false;
  return backend->replace(nullptr, 0);
}

uint32_t studyDay(uint32_t unix) {
  const uint32_t shift = (uint32_t)StudyRules::kStudyDayStartHour * 3600u;
  return unix < shift ? 0 : (unix - shift) / 86400u;
}

uint8_t weekdayOfStudyDay(uint32_t day) { return (uint8_t)((day + 4) % 7); }   // 1970-01-01 was a Thursday

}  // namespace SessionLog
