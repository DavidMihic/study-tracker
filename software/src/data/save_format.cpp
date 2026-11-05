#include "data/save_format.h"

namespace SaveFormat {

namespace {

constexpr uint8_t kMarker[4] = {'S', 'T', 'U', 'D'};
constexpr size_t kHeaderBytes = 8;
constexpr size_t kCrcBytes = 4;

// Little helpers that write/read numbers byte by byte, so the layout does not depend on the CPU.
class Writer {
 public:
  Writer(uint8_t* buffer, size_t capacity) : buffer_(buffer), capacity_(capacity) {}
  void u8(uint8_t v) { if (length_ < capacity_) buffer_[length_]=v; length_++; }
  void u16(uint16_t v) { u8((uint8_t)v); u8((uint8_t)(v >> 8)); }
  void u32(uint32_t v) { u16((uint16_t)v); u16((uint16_t)(v >> 16)); }
  size_t length() const { return length_; }
  bool overflowed() const { return length_ > capacity_; }
 private:
  uint8_t* buffer_;
  size_t capacity_;
  size_t length_ = 0;
};

class Reader {
 public:
  Reader(const uint8_t* bytes, size_t length) : bytes_(bytes), length_(length) {}
  uint8_t u8() { if (pos_ >= length_) { failed_ = true; return 0; } return bytes_[pos_++]; }
  uint16_t u16() { uint16_t lo = u8(); return (uint16_t)(lo | (u8() << 8)); }
  uint32_t u32() { uint32_t lo = u16(); return lo | ((uint32_t)u16() << 16); }
  bool failed() const { return failed_; }
  size_t position() const { return pos_; }
 private:
  const uint8_t* bytes_;
  size_t length_;
  size_t pos_ = 0;
  bool failed_ = false;
};

}  // namespace

uint32_t crc32(const uint8_t* bytes, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; i++) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

size_t serialize(const Data& data, uint8_t* out, size_t capacity) {
  Writer w(out, capacity);
  for (uint8_t c : kMarker) w.u8(c);
  w.u8(kVersion);
  w.u8(0);                 // reserved
  w.u16(0);                // payload length, filled in below

  w.u8(data.semester);
  w.u16(data.targetMinutes);
  w.u8(data.brightness);
  w.u8((uint8_t)((data.soundTurn ? 1 : 0) | (data.soundPress ? 2 : 0) | (data.soundNudge ? 4 : 0) |
                 (data.soundRestore ? 0 : 8)));
  w.u16(data.lastSubjectId);

  w.u8(data.active.active ? 1 : 0);
  w.u16(data.active.subjectId);
  w.u32(data.active.accumulatedSeconds);
  w.u32(data.active.runStartUnix);
  w.u8(data.active.running ? 1 : 0);
  w.u32(data.active.checkpointUnix);

  w.u16(data.subjectCount);
  for (uint16_t i = 0; i < data.subjectCount; i++) {
    const SubjectRecord& r = data.subjects[i];
    w.u16(r.id);
    w.u32(r.totalSeconds);
    w.u16(r.sessions);
    w.u8(r.favorite ? 1 : 0);
  }

  size_t payload = w.length() - kHeaderBytes;
  if (w.length() + kCrcBytes > capacity) return 0;
  out[6] = (uint8_t)payload;
  out[7] = (uint8_t)(payload >> 8);

  uint32_t crc = crc32(out, w.length());
  w.u32(crc);
  return w.length();
}

bool parse(const uint8_t* bytes, size_t length, Data& data) {
  if (length < kHeaderBytes + kCrcBytes) return false;
  for (size_t i = 0; i < 4; i++) if (bytes[i] != kMarker[i]) return false;
  if (bytes[4] != kVersion) return false;

  size_t payload = (size_t)bytes[6] | ((size_t)bytes[7] << 8);
  size_t total = kHeaderBytes + payload + kCrcBytes;
  if (total > length) return false;

  Reader crcReader(bytes + kHeaderBytes + payload, kCrcBytes);
  if (crc32(bytes, kHeaderBytes + payload) != crcReader.u32()) return false;

  Reader r(bytes + kHeaderBytes, payload);
  data.semester = r.u8();
  data.targetMinutes = r.u16();
  data.brightness = r.u8();
  uint8_t sounds = r.u8();
  data.soundTurn = sounds & 1;
  data.soundPress = sounds & 2;
  data.soundNudge = sounds & 4;
  data.soundRestore = !(sounds & 8);
  data.lastSubjectId = r.u16();

  data.active.active = r.u8() != 0;
  data.active.subjectId = r.u16();
  data.active.accumulatedSeconds = r.u32();
  data.active.runStartUnix = r.u32();
  data.active.running = r.u8() != 0;
  data.active.checkpointUnix = r.u32();

  data.subjectCount = r.u16();
  if (data.subjectCount > Subjects::kMaxSubjects) return false;
  for (uint16_t i = 0; i < data.subjectCount; i++) {
    SubjectRecord& s = data.subjects[i];
    s.id = r.u16();
    s.totalSeconds = r.u32();
    s.sessions = r.u16();
    s.favorite = r.u8() != 0;
  }
  return !r.failed() && r.position() == payload;
}

}  // namespace SaveFormat
