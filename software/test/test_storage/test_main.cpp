// PC tests for saving to flash: the byte format (including damaged data) and the rules for WHEN and WHAT
// is written. The flash is replaced by an array in memory. Run with:  pio test -e native
#include <string.h>
#include <unity.h>
#include <vector>

#include "app/app_state.h"
#include "app/persistence.h"
#include "app/stats.h"
#include "ui/format.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "data/save_format.h"
#include "data/session_log.h"
#include "data/subjects.h"

// Subject positions in the fixture list (test/support/test_subjects.cpp); ids are position + 1.
enum : size_t { kA1 = 0, kA2 = 1, kW1 = 2, kS1 = 3, kB1 = 4, kE1 = 5, kE2 = 6, kE3 = 7 };

namespace {

// ---- fake flash and fake clock
class MemoryBackend : public Persistence::Backend {
 public:
  std::vector<uint8_t> bytes;
  int writes = 0;
  size_t read(uint8_t* buffer, size_t capacity) override {
    size_t n = bytes.size() < capacity ? bytes.size() : capacity;
    if (n) memcpy(buffer, bytes.data(), n);
    return n;
  }
  bool write(const uint8_t* data, size_t length) override {
    bytes.assign(data, data + length);
    writes++;
    return true;
  }
};

uint32_t gMs = 0, gUnix = 0;
uint32_t fakeMs() { return gMs; }
uint32_t fakeUnix() { return gUnix; }

MemoryBackend flash;

void startPersistence() { Persistence::begin(&flash, {fakeMs, fakeUnix}); }

// Simulates a power cut and reboot: RAM is forgotten, only `flash` survives.
bool reboot() {
  Progress::reset();
  AppState::session() = Session();
  startPersistence();
  bool loaded = Persistence::load();
  Persistence::restoreSession();   // the clock is "running" again: fakeUnix() gives the new time
  return loaded;
}

}  // namespace

void setUp() {
  Progress::reset();
  AppState::session() = Session();
  flash.bytes.clear();
  flash.writes = 0;
  gMs = 0;
  gUnix = 1000000;
  startPersistence();
}
void tearDown() {}

// ---------------------------------------------------------------- byte format
void test_crc32_known_value() {
  const uint8_t text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, SaveFormat::crc32(text, sizeof text));
}

static SaveFormat::Data sampleData() {
  SaveFormat::Data d = {};
  d.semester = 6;
  d.targetMinutes = 40;
  d.brightness = 130;
  d.soundTurn = false;
  d.soundPress = true;
  d.soundNudge = false;
  d.lastSubjectId = 3;
  d.active = {true, 5, 1234, 99999, true, 100500};
  d.subjectCount = 2;
  d.subjects[0] = {1, 7200, 4, true};
  d.subjects[1] = {8, 65, 1, false};
  return d;
}

void test_format_round_trip() {
  SaveFormat::Data in = sampleData();
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(in, buf, sizeof buf);
  TEST_ASSERT_TRUE(n > 0);

  SaveFormat::Data out = {};
  TEST_ASSERT_TRUE(SaveFormat::parse(buf, n, out));
  TEST_ASSERT_EQUAL(6, out.semester);
  TEST_ASSERT_EQUAL(40, out.targetMinutes);
  TEST_ASSERT_EQUAL(130, out.brightness);
  TEST_ASSERT_FALSE(out.soundTurn);
  TEST_ASSERT_TRUE(out.soundPress);
  TEST_ASSERT_FALSE(out.soundNudge);
  TEST_ASSERT_EQUAL(3, out.lastSubjectId);
  TEST_ASSERT_TRUE(out.active.active);
  TEST_ASSERT_EQUAL(5, out.active.subjectId);
  TEST_ASSERT_EQUAL(1234, out.active.accumulatedSeconds);
  TEST_ASSERT_EQUAL(99999, out.active.runStartUnix);
  TEST_ASSERT_TRUE(out.active.running);
  TEST_ASSERT_EQUAL(100500, out.active.checkpointUnix);
  TEST_ASSERT_EQUAL(2, out.subjectCount);
  TEST_ASSERT_EQUAL(7200, out.subjects[0].totalSeconds);
  TEST_ASSERT_TRUE(out.subjects[0].favorite);
  TEST_ASSERT_EQUAL(8, out.subjects[1].id);
  TEST_ASSERT_EQUAL(1, out.subjects[1].sessions);
}

// The save data for the largest possible list (kMaxSubjects records) must fit the buffer and read back whole.
void test_format_full_table_round_trip() {
  SaveFormat::Data in = sampleData();
  in.subjectCount = Subjects::kMaxSubjects;
  for (uint16_t i = 0; i < Subjects::kMaxSubjects; i++) in.subjects[i] = {(uint16_t)(i + 1), 100000u + i, (uint16_t)(i + 3), (i % 2) == 0};
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(in, buf, sizeof buf);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_TRUE(n <= SaveFormat::kMaxBytes);
  TEST_ASSERT_EQUAL(SaveFormat::kMaxBytes, n);

  SaveFormat::Data out = {};
  TEST_ASSERT_TRUE(SaveFormat::parse(buf, n, out));
  TEST_ASSERT_EQUAL(Subjects::kMaxSubjects, out.subjectCount);
  for (uint16_t i = 0; i < Subjects::kMaxSubjects; i++) {
    TEST_ASSERT_EQUAL(in.subjects[i].id, out.subjects[i].id);
    TEST_ASSERT_EQUAL(in.subjects[i].totalSeconds, out.subjects[i].totalSeconds);
    TEST_ASSERT_EQUAL(in.subjects[i].sessions, out.subjects[i].sessions);
    TEST_ASSERT_EQUAL(in.subjects[i].favorite, out.subjects[i].favorite);
  }
}

void test_format_rejects_damaged_data() {
  SaveFormat::Data in = sampleData(), out = {};
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(in, buf, sizeof buf);

  TEST_ASSERT_FALSE(SaveFormat::parse(buf, 0, out));            // nothing
  TEST_ASSERT_FALSE(SaveFormat::parse(buf, n - 1, out));        // cut short
  TEST_ASSERT_FALSE(SaveFormat::parse(buf, 5, out));

  uint8_t copy[SaveFormat::kMaxBytes];
  memcpy(copy, buf, n);
  copy[20] ^= 0x01;                                             // one flipped bit in the payload
  TEST_ASSERT_FALSE(SaveFormat::parse(copy, n, out));

  memcpy(copy, buf, n);
  copy[0] = 'X';                                                // wrong marker
  TEST_ASSERT_FALSE(SaveFormat::parse(copy, n, out));

  memcpy(copy, buf, n);
  copy[4] = SaveFormat::kVersion + 1;                           // a version this firmware does not know
  TEST_ASSERT_FALSE(SaveFormat::parse(copy, n, out));

  memset(copy, 0xFF, sizeof copy);                              // blank flash
  TEST_ASSERT_FALSE(SaveFormat::parse(copy, 4096 < sizeof copy ? 4096 : sizeof copy, out));
}

void test_format_too_small_buffer_is_refused() {
  SaveFormat::Data in = sampleData();
  uint8_t small[16];
  TEST_ASSERT_EQUAL(0, SaveFormat::serialize(in, small, sizeof small));
}

// ---------------------------------------------------------------- save and load
void test_everything_survives_a_reboot() {
  Progress::setCurrentSemester(6);
  Progress::setTargetMinutes(45);
  Progress::setBrightness(120);
  Progress::setSoundTurn(false);
  Progress::setSoundNudge(false);
  Progress::recordSession(kA1, 1500);
  Progress::recordSession(kA1, 900);
  Progress::recordSession(kE2, 120);
  Progress::setFavorite(kW1, true);
  Progress::setLastSubject(kB1);
  Persistence::saveNow();

  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_EQUAL(6, Progress::currentSemester());
  TEST_ASSERT_EQUAL(45, Progress::targetMinutes());
  TEST_ASSERT_EQUAL(120, Progress::brightness());
  TEST_ASSERT_FALSE(Progress::soundTurn());
  TEST_ASSERT_TRUE(Progress::soundPress());
  TEST_ASSERT_FALSE(Progress::soundNudge());
  TEST_ASSERT_EQUAL(2400, Progress::stats(kA1).totalSeconds);
  TEST_ASSERT_EQUAL(2, Progress::stats(kA1).sessions);
  TEST_ASSERT_EQUAL(120, Progress::stats(kE2).totalSeconds);
  TEST_ASSERT_TRUE(Progress::stats(kW1).favorite);
  TEST_ASSERT_EQUAL(0, Progress::stats(kA2).totalSeconds);
  TEST_ASSERT_EQUAL(kB1, Progress::lastSubjectIndex());
}

void test_restore_alert_setting_survives_and_old_data_reads_as_on() {
  Progress::setSoundRestore(false);
  Persistence::saveNow();
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_FALSE(Progress::soundRestore());

  // Data that was written before this switch existed has bit 3 clear: that must read as ON.
  SaveFormat::Data d = {};
  d.soundTurn = d.soundPress = d.soundNudge = true;
  d.soundRestore = true;                                        // serialized as bit 3 = 0
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(d, buf, sizeof buf);
  flash.bytes.assign(buf, buf + n);
  Progress::reset();
  Progress::setSoundRestore(false);
  TEST_ASSERT_TRUE(Persistence::load());
  TEST_ASSERT_TRUE(Progress::soundRestore());
}

void test_blank_flash_gives_defaults() {
  flash.bytes.assign(4096, 0xFF);
  TEST_ASSERT_FALSE(Persistence::load());
  TEST_ASSERT_EQUAL(5, Progress::currentSemester());
  TEST_ASSERT_EQUAL(25, Progress::targetMinutes());
  TEST_ASSERT_EQUAL(Session::Phase::Idle, AppState::session().phase());
  TEST_ASSERT_FALSE(Persistence::restoredSession());
}

void test_damaged_flash_changes_nothing() {
  Progress::recordSession(kA1, 600);
  Persistence::saveNow();
  flash.bytes[12] ^= 0x40;                                      // damage
  Progress::reset();
  TEST_ASSERT_FALSE(Persistence::load());
  TEST_ASSERT_EQUAL(0, Progress::stats(kA1).totalSeconds);      // nothing half-loaded
  TEST_ASSERT_EQUAL(5, Progress::currentSemester());
}

void test_subjects_missing_from_the_list_are_skipped() {
  SaveFormat::Data d = sampleData();
  d.active.active = false;
  d.subjectCount = 2;
  d.subjects[0] = {999, 5000, 3, true};     // an id that is not in the subject list
  d.subjects[1] = {2, 300, 1, false};       // A2
  d.lastSubjectId = 999;
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(d, buf, sizeof buf);
  flash.bytes.assign(buf, buf + n);

  TEST_ASSERT_TRUE(Persistence::load());
  TEST_ASSERT_EQUAL(300, Progress::stats(kA2).totalSeconds);
  TEST_ASSERT_EQUAL(-1, Progress::lastSubjectIndex());
}

// ---------------------------------------------------------------- a running block
void test_running_block_comes_back_paused_with_checkpoint_time() {
  gUnix = 5000;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();                                       // checkpoint at 5000

  gUnix = 5000 + Persistence::kCheckpointSeconds;               // 10 minutes later
  Persistence::update();                                        // checkpoint written: 5600

  gUnix += 3600;                                                // the power is cut and stays off for an hour
  TEST_ASSERT_TRUE(reboot());

  Session& s = AppState::session();
  TEST_ASSERT_EQUAL(Session::Phase::Studying, s.phase());
  TEST_ASSERT_TRUE(s.isPaused());
  TEST_ASSERT_EQUAL(kA1, s.subjectIndex());
  TEST_ASSERT_EQUAL(600, s.elapsedSeconds(gUnix));              // long gap: only up to the checkpoint counts
  TEST_ASSERT_EQUAL(600, s.elapsedSeconds(gUnix + 99999));      // paused: stays
  TEST_ASSERT_EQUAL(1500, s.targetSeconds());
  TEST_ASSERT_TRUE(Persistence::restoredSession());

  // She continues: counting goes on from there.
  s.resume(gUnix + 1000);
  TEST_ASSERT_EQUAL(660, s.elapsedSeconds(gUnix + 1060));
}

void test_short_interruption_counts_the_time_that_passed() {
  gUnix = 5000;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();                                       // checkpoint at 5000, nothing newer is saved
  gUnix = 5000 + 180;                                           // unplugged and plugged back in after 3 minutes
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_TRUE(AppState::session().isPaused());
  TEST_ASSERT_EQUAL(180, AppState::session().elapsedSeconds(gUnix));   // not 00:00
}

void test_gap_limit_and_missing_or_backwards_clock() {
  const uint32_t limit = StudyRules::kResumeTrustSeconds;
  gUnix = 5000;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();

  gUnix = 5000 + limit;                                         // exactly at the limit: still trusted
  reboot();
  TEST_ASSERT_EQUAL(limit, AppState::session().elapsedSeconds(gUnix));

  gUnix = 5000 + limit + 1;                                     // one second later: only the checkpoint counts
  reboot();
  TEST_ASSERT_EQUAL(0, AppState::session().elapsedSeconds(gUnix));

  gUnix = 0;                                                    // clock not available
  reboot();
  TEST_ASSERT_EQUAL(0, AppState::session().elapsedSeconds(5000 + 100));

  gUnix = 4000;                                                 // clock went backwards (e.g. was reset)
  reboot();
  TEST_ASSERT_EQUAL(0, AppState::session().elapsedSeconds(4000));
}

void test_power_cut_alert_only_when_the_block_was_running() {
  gUnix = 100;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();
  gUnix = 160;
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_TRUE(Persistence::restoredSession());
  TEST_ASSERT_TRUE(Persistence::restoredWasRunning());          // the cut stopped it: she must be told
  Persistence::acknowledgeRestored();
  TEST_ASSERT_FALSE(Persistence::restoredWasRunning());

  AppState::session().resume(gUnix);
  AppState::session().pause(gUnix + 50);                        // she paused it herself
  Persistence::saveNow();
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_TRUE(Persistence::restoredSession());
  TEST_ASSERT_FALSE(Persistence::restoredWasRunning());         // no alert: she knows it is paused
}

void test_paused_block_keeps_its_time() {
  gUnix = 100;
  AppState::session().startStudy(gUnix, kB1, 1500);
  AppState::session().pause(gUnix + 400);
  gUnix += 400;
  Persistence::saveNow();
  gUnix += 86400;                                               // the device stays off for a day

  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_TRUE(AppState::session().isPaused());
  TEST_ASSERT_EQUAL(400, AppState::session().elapsedSeconds(gUnix));
}

void test_restored_block_does_not_repeat_the_nudge_that_already_came() {
  gUnix = 0;
  AppState::session().startStudy(gUnix, kA1, 600);
  gUnix = 900;
  AppState::session().checkpoint(gUnix);
  Persistence::saveNow();
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_FALSE(AppState::session().nudgeDue(gUnix));       // 900 s counted > 600 s target
}

void test_finished_block_leaves_nothing_to_restore() {
  gUnix = 100;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();
  uint32_t length = AppState::session().endStudy(gUnix + 1800);
  Progress::recordSession(kA1, length);
  Persistence::saveNow();                                       // what the Studying screen does on stop

  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_EQUAL(Session::Phase::Idle, AppState::session().phase());
  TEST_ASSERT_FALSE(Persistence::restoredSession());
  TEST_ASSERT_EQUAL(1800, Progress::stats(kA1).totalSeconds);
}

void test_block_of_a_removed_subject_is_not_restored() {
  SaveFormat::Data d = sampleData();
  d.active = {true, 999, 100, 100, true, 200};
  uint8_t buf[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(d, buf, sizeof buf);
  flash.bytes.assign(buf, buf + n);
  TEST_ASSERT_TRUE(Persistence::load());
  TEST_ASSERT_EQUAL(Session::Phase::Idle, AppState::session().phase());
}

// ---------------------------------------------------------------- when it writes
void test_requested_save_waits_until_things_are_quiet() {
  Progress::setBrightness(100);
  Persistence::requestSave();
  gMs = 1999;
  Persistence::update();
  TEST_ASSERT_EQUAL(0, flash.writes);

  Progress::setBrightness(90);                                  // dial still turning: the timer restarts
  Persistence::requestSave();
  gMs = 1999 + 1999;
  Persistence::update();
  TEST_ASSERT_EQUAL(0, flash.writes);

  gMs = 1999 + 2000;
  Persistence::update();
  TEST_ASSERT_EQUAL(1, flash.writes);                           // one write for the whole spin
  Persistence::update();
  TEST_ASSERT_EQUAL(1, flash.writes);                           // and nothing more while nothing changes

  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_EQUAL(90, Progress::brightness());
}

void test_save_now_writes_immediately() {
  Persistence::saveNow();
  TEST_ASSERT_EQUAL(1, flash.writes);
}

void test_checkpoints_only_while_a_block_is_running() {
  gUnix = 1000;
  AppState::session().startStudy(gUnix, kA1, 1500);
  Persistence::saveNow();
  TEST_ASSERT_EQUAL(1, flash.writes);

  gUnix = 1000 + Persistence::kCheckpointSeconds - 1;
  Persistence::update();
  TEST_ASSERT_EQUAL(1, flash.writes);                           // not yet
  gUnix += 1;
  Persistence::update();
  TEST_ASSERT_EQUAL(2, flash.writes);                           // 10 minutes: checkpoint

  AppState::session().pause(gUnix);
  Persistence::saveNow();
  int before = flash.writes;
  gUnix += 3600;
  Persistence::update();
  TEST_ASSERT_EQUAL(before, flash.writes);                      // paused: no checkpoints, no wear
}

void test_no_checkpoints_when_idle_or_without_clock() {
  gUnix += 100000;
  Persistence::update();
  TEST_ASSERT_EQUAL(0, flash.writes);                           // idle

  AppState::session().startStudy(gUnix, kA1, 1500);
  gUnix = 0;                                                    // clock not available
  Persistence::update();
  TEST_ASSERT_EQUAL(0, flash.writes);
}

// ---------------------------------------------------------------- block start time (format version 2)
void test_block_start_time_survives_a_reboot() {
  gUnix = 5000;
  AppState::session().startStudy(gUnix, kA1, 1500);
  gUnix = 5600;
  AppState::session().pause(gUnix);
  gUnix = 6000;
  AppState::session().resume(gUnix);       // a later running stretch must not change the start
  Persistence::saveNow();
  TEST_ASSERT_TRUE(reboot());
  TEST_ASSERT_EQUAL(5000, AppState::session().startedUnix());
}

void test_version_1_data_is_still_read() {
  SaveFormat::Data d = sampleData();
  d.active = {true, 5, 1234, 99999, true, 100500, 77777};
  uint8_t v2[SaveFormat::kMaxBytes];
  size_t n = SaveFormat::serialize(d, v2, sizeof v2);

  // Build the version 1 bytes: no startedUnix field (4 bytes at offset 8 + 23), version 1, new length and CRC.
  const size_t cut = 8 + 23;
  std::vector<uint8_t> v1(v2, v2 + cut);
  v1.insert(v1.end(), v2 + cut + 4, v2 + n - 4);
  v1[4] = 1;
  size_t payload = v1.size() - 8;
  v1[6] = (uint8_t)payload;
  v1[7] = (uint8_t)(payload >> 8);
  uint32_t crc = SaveFormat::crc32(v1.data(), v1.size());
  for (int i = 0; i < 4; i++) v1.push_back((uint8_t)(crc >> (8 * i)));

  SaveFormat::Data out = {};
  TEST_ASSERT_TRUE(SaveFormat::parse(v1.data(), v1.size(), out));
  TEST_ASSERT_EQUAL(99999, out.active.runStartUnix);
  TEST_ASSERT_EQUAL(99999, out.active.startedUnix);   // falls back to the start of the running stretch
  TEST_ASSERT_EQUAL(100500, out.active.checkpointUnix);
  TEST_ASSERT_EQUAL(d.subjectCount, out.subjectCount);
}

// ---------------------------------------------------------------- session log
namespace {

class MemoryLogBackend : public SessionLog::Backend {
 public:
  std::vector<uint8_t> bytes;
  size_t size() override { return bytes.size(); }
  size_t failReadsAfter = (size_t)-1;   // reads that start at or after this offset fail (a flash error)
  size_t read(size_t offset, uint8_t* buffer, size_t length) override {
    if (offset >= failReadsAfter) return 0;
    if (offset >= bytes.size()) return 0;
    size_t n = bytes.size() - offset < length ? bytes.size() - offset : length;
    memcpy(buffer, bytes.data() + offset, n);
    return n;
  }
  bool append(const uint8_t* data, size_t length) override { bytes.insert(bytes.end(), data, data + length); return true; }
  bool replace(const uint8_t* data, size_t length) override { bytes.assign(data, data + length); return true; }
};

MemoryLogBackend logFlash;

void collect(const SessionLog::Record& r, void* context) {
  static_cast<std::vector<SessionLog::Record>*>(context)->push_back(r);
}

std::vector<SessionLog::Record> allRecords() {
  std::vector<SessionLog::Record> v;
  SessionLog::forEach(collect, &v);
  return v;
}

}  // namespace

void test_log_appends_and_reads_back_in_order() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  TEST_ASSERT_EQUAL(0, SessionLog::count());
  TEST_ASSERT_TRUE(SessionLog::append({1000, 1500, 3}));
  TEST_ASSERT_TRUE(SessionLog::append({9000, 60, 4}));
  TEST_ASSERT_EQUAL(2 * SessionLog::kRecordBytes, logFlash.bytes.size());

  auto records = allRecords();
  TEST_ASSERT_EQUAL(2, records.size());
  TEST_ASSERT_EQUAL(1000, records[0].startUnix);
  TEST_ASSERT_EQUAL(1500, records[0].seconds);
  TEST_ASSERT_EQUAL(3, records[0].subjectId);
  TEST_ASSERT_EQUAL(9000, records[1].startUnix);
  TEST_ASSERT_EQUAL(4, records[1].subjectId);
}

void test_log_skips_a_damaged_record_and_repairs_a_torn_tail() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  SessionLog::append({1000, 100, 1});
  SessionLog::append({2000, 200, 2});
  SessionLog::append({3000, 300, 3});
  logFlash.bytes[SessionLog::kRecordBytes + 2] ^= 0x40;       // flip a bit in the middle record
  auto records = allRecords();
  TEST_ASSERT_EQUAL(2, records.size());
  TEST_ASSERT_EQUAL(1000, records[0].startUnix);
  TEST_ASSERT_EQUAL(3000, records[1].startUnix);

  logFlash.bytes.resize(logFlash.bytes.size() - 5);           // power cut in the middle of the last record
  SessionLog::begin(&logFlash);                               // reboot
  TEST_ASSERT_EQUAL(0, logFlash.bytes.size() % SessionLog::kRecordBytes);
  SessionLog::append({4000, 400, 4});                         // lands aligned, readable
  records = allRecords();
  TEST_ASSERT_EQUAL(2, records.size());                       // 1000 (the damaged middle one is gone) and 4000
  TEST_ASSERT_EQUAL(4000, records[1].startUnix);
}

// A read error half-way through the log must never be written back as a shorter log.
void test_log_is_not_shortened_when_a_read_fails() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  for (uint16_t i = 0; i < 40; i++) SessionLog::append({1000000u + i, 600, (uint16_t)(1 + i % 3)});
  const std::vector<uint8_t> before = logFlash.bytes;
  logFlash.failReadsAfter = 12 * 32;   // the first 32-record chunk reads fine, the next one fails
  TEST_ASSERT_FALSE(SessionLog::removeSubject(1));
  logFlash.failReadsAfter = (size_t)-1;
  TEST_ASSERT_TRUE(before == logFlash.bytes);
  TEST_ASSERT_EQUAL(40, SessionLog::count());
}

void test_log_removes_one_subject_or_everything() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  SessionLog::append({1000, 100, 1});
  SessionLog::append({2000, 200, 2});
  SessionLog::append({3000, 300, 1});
  TEST_ASSERT_TRUE(SessionLog::removeSubject(1));
  auto records = allRecords();
  TEST_ASSERT_EQUAL(1, records.size());
  TEST_ASSERT_EQUAL(2, records[0].subjectId);
  TEST_ASSERT_TRUE(SessionLog::clear());
  TEST_ASSERT_EQUAL(0, SessionLog::count());
}

void test_study_day_starts_at_5_am() {
  // 2026-11-04 is a Wednesday. Midnight of that day, as a unix time with the RTC's "local = UTC" convention:
  const uint32_t wedMidnight = 1793750400u;
  TEST_ASSERT_EQUAL(3, SessionLog::weekdayOfStudyDay(SessionLog::studyDay(wedMidnight + 12 * 3600)));
  uint32_t wednesdayEvening = SessionLog::studyDay(wedMidnight + 22 * 3600);
  uint32_t thursdayOneAm = SessionLog::studyDay(wedMidnight + 25 * 3600);
  uint32_t thursdayFourFiftyNine = SessionLog::studyDay(wedMidnight + 28 * 3600 + 59 * 60);
  uint32_t thursdayFive = SessionLog::studyDay(wedMidnight + 29 * 3600);
  TEST_ASSERT_EQUAL(wednesdayEvening, thursdayOneAm);                 // past midnight still Wednesday
  TEST_ASSERT_EQUAL(wednesdayEvening, thursdayFourFiftyNine);
  TEST_ASSERT_EQUAL(wednesdayEvening + 1, thursdayFive);              // 05:00 starts Thursday
  TEST_ASSERT_EQUAL(4, SessionLog::weekdayOfStudyDay(thursdayFive));  // 4 = Thursday
}

// ---------------------------------------------------------------- statistics
namespace {

constexpr uint32_t kWedMidnight = 1793750400u;   // Wednesday 2026-11-04 00:00
constexpr uint32_t kHour = 3600, kDay = 86400;

uint32_t studyDayAt(int dayOffsetFromWed, int hour) {
  return SessionLog::studyDay(kWedMidnight + dayOffsetFromWed * kDay + hour * kHour);
}

// A week with a block that runs past midnight, a block started after midnight (still the same study day),
// two subjects and a block in the following week.
void fillSampleWeek() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  SessionLog::append({kWedMidnight - 2 * kDay + 10 * kHour, 1000, 1});   // Monday
  SessionLog::append({kWedMidnight + 22 * kHour, 3000, 2});              // Wednesday evening, ends after midnight
  SessionLog::append({kWedMidnight + 23 * kHour + 1800, 600, 1});        // Wednesday 23:30
  SessionLog::append({kWedMidnight + kDay + 3 * kHour, 300, 1});         // Thursday 03:00 = still Wednesday
  SessionLog::append({kWedMidnight + kDay + 10 * kHour, 700, 2});        // Thursday
  SessionLog::append({kWedMidnight + 4 * kDay + 12 * kHour, 100, 1});    // Sunday
  SessionLog::append({kWedMidnight + 5 * kDay + 9 * kHour, 50, 1});      // next Monday
}

}  // namespace

void test_week_runs_monday_to_sunday() {
  const uint32_t monday = studyDayAt(-2, 12);
  for (int offset = -2; offset <= 4; offset++) {                          // Monday .. Sunday
    TEST_ASSERT_EQUAL(monday, Stats::weekStart(studyDayAt(offset, 12)));
    TEST_ASSERT_EQUAL(monday + 6, Stats::weekEnd(studyDayAt(offset, 12)));
  }
  TEST_ASSERT_EQUAL(monday + 7, Stats::weekStart(studyDayAt(5, 12)));     // next Monday starts a new week
}

void test_stats_total_for_a_week() {
  fillSampleWeek();
  Stats::Totals t = Stats::total(Stats::weekOf(studyDayAt(0, 12)));
  TEST_ASSERT_EQUAL(5700, t.seconds);       // 1000 + 3000 + 600 + 300 + 700 + 100; the next week's 50 is out
  TEST_ASSERT_EQUAL(6, t.sessions);
}

void test_stats_per_day_puts_a_block_on_the_day_it_started() {
  fillSampleWeek();
  Stats::Totals days[7];
  Stats::perDay(Stats::weekStart(studyDayAt(0, 12)), 7, days);
  TEST_ASSERT_EQUAL(1000, days[0].seconds);   // Monday
  TEST_ASSERT_EQUAL(1, days[0].sessions);
  TEST_ASSERT_EQUAL(0, days[1].seconds);      // Tuesday: nothing
  TEST_ASSERT_EQUAL(0, days[1].sessions);
  TEST_ASSERT_EQUAL(3900, days[2].seconds);   // Wednesday: 3000 + 600 + the 300 started at 03:00 Thursday
  TEST_ASSERT_EQUAL(3, days[2].sessions);
  TEST_ASSERT_EQUAL(700, days[3].seconds);    // Thursday, only what started after 05:00
  TEST_ASSERT_EQUAL(0, days[4].seconds);
  TEST_ASSERT_EQUAL(0, days[5].seconds);
  TEST_ASSERT_EQUAL(100, days[6].seconds);    // Sunday
}

void test_stats_per_subject_sorted_by_time() {
  fillSampleWeek();
  Stats::SubjectTotal rows[8];
  size_t n = Stats::perSubject(Stats::weekOf(studyDayAt(0, 12)), rows, 8);
  TEST_ASSERT_EQUAL(2, n);
  TEST_ASSERT_EQUAL(2, rows[0].subjectId);    // 3000 + 700
  TEST_ASSERT_EQUAL(3700, rows[0].seconds);
  TEST_ASSERT_EQUAL(2, rows[0].sessions);
  TEST_ASSERT_EQUAL(1, rows[1].subjectId);    // 1000 + 600 + 300 + 100
  TEST_ASSERT_EQUAL(2000, rows[1].seconds);
  TEST_ASSERT_EQUAL(4, rows[1].sessions);

  Stats::SubjectTotal one[1];                 // a small buffer is never overrun
  TEST_ASSERT_EQUAL(1, Stats::perSubject(Stats::weekOf(studyDayAt(0, 12)), one, 1));
}

void test_stats_with_an_empty_log() {
  logFlash.bytes.clear();
  SessionLog::begin(&logFlash);
  Stats::Totals t = Stats::total({0, 100000});
  TEST_ASSERT_EQUAL(0, t.seconds);
  TEST_ASSERT_EQUAL(0, t.sessions);
  Stats::SubjectTotal rows[2];
  TEST_ASSERT_EQUAL(0, Stats::perSubject({0, 100000}, rows, 2));
}

void test_format_bar_label() {
  const struct { uint32_t seconds; const char* expected; } cases[] = {
    {60, "1min"}, {29 * 60 + 40, "30min"}, {47 * 60, "47min"}, {59 * 60 + 29, "59min"},
    {59 * 60 + 30, "1h"},                                   // would be "60min": rounds to the hour instead
    {60 * 60, "1h"}, {74 * 60 + 59, "1h"}, {75 * 60, "1,5h"}, {104 * 60, "1,5h"}, {105 * 60, "2h"},
    {2 * 3600 + 10 * 60, "2h"}, {2 * 3600 + 20 * 60, "2,5h"}, {2 * 3600 + 50 * 60, "3h"}, {5 * 3600, "5h"}, {11 * 3600 + 40 * 60, "11,5h"},
  };
  char text[16];
  for (const auto& c : cases) {
    Format::barLabel(text, sizeof text, c.seconds);
    TEST_ASSERT_EQUAL_STRING(c.expected, text);
  }
}

void test_format_block_plural_and_dates() {
  char text[32];
  const struct { uint32_t count; const char* expected; } plural[] = {
    {0, "0 blokova"}, {1, "1 blok"}, {2, "2 bloka"}, {4, "4 bloka"}, {5, "5 blokova"},
    {11, "11 blokova"}, {12, "12 blokova"}, {21, "21 blok"}, {22, "22 bloka"}, {111, "111 blokova"},
  };
  for (const auto& p : plural) {
    Format::blocks(text, sizeof text, p.count);
    TEST_ASSERT_EQUAL_STRING(p.expected, text);
  }

  const uint32_t wednesday = kWedMidnight / kDay;          // day number of 2026-11-04
  Format::dayMonth(text, sizeof text, wednesday);
  TEST_ASSERT_EQUAL_STRING("4. 11.", text);
  Format::dayRange(text, sizeof text, wednesday - 2, wednesday + 4);
  TEST_ASSERT_EQUAL_STRING("2. 11. - 8. 11.", text);
  Format::dayRange(text, sizeof text, wednesday + 27, wednesday + 33);
  TEST_ASSERT_EQUAL_STRING("1. 12. - 7. 12.", text);
  Format::dayRange(text, sizeof text, 1798675200u / kDay, 1798761600u / kDay);   // 2026-12-31 to 2027-01-01
  TEST_ASSERT_EQUAL_STRING("31. 12. - 1. 1.", text);
  Format::dayMonth(text, sizeof text, (1709164800u / kDay));   // 2024-02-29 (leap day)
  TEST_ASSERT_EQUAL_STRING("29. 2.", text);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_format_bar_label);
  RUN_TEST(test_format_block_plural_and_dates);
  RUN_TEST(test_week_runs_monday_to_sunday);
  RUN_TEST(test_stats_total_for_a_week);
  RUN_TEST(test_stats_per_day_puts_a_block_on_the_day_it_started);
  RUN_TEST(test_stats_per_subject_sorted_by_time);
  RUN_TEST(test_stats_with_an_empty_log);
  RUN_TEST(test_block_start_time_survives_a_reboot);
  RUN_TEST(test_version_1_data_is_still_read);
  RUN_TEST(test_log_appends_and_reads_back_in_order);
  RUN_TEST(test_log_skips_a_damaged_record_and_repairs_a_torn_tail);
  RUN_TEST(test_log_is_not_shortened_when_a_read_fails);
  RUN_TEST(test_log_removes_one_subject_or_everything);
  RUN_TEST(test_study_day_starts_at_5_am);
  RUN_TEST(test_crc32_known_value);
  RUN_TEST(test_format_round_trip);
  RUN_TEST(test_format_full_table_round_trip);
  RUN_TEST(test_format_rejects_damaged_data);
  RUN_TEST(test_format_too_small_buffer_is_refused);
  RUN_TEST(test_everything_survives_a_reboot);
  RUN_TEST(test_restore_alert_setting_survives_and_old_data_reads_as_on);
  RUN_TEST(test_blank_flash_gives_defaults);
  RUN_TEST(test_damaged_flash_changes_nothing);
  RUN_TEST(test_subjects_missing_from_the_list_are_skipped);
  RUN_TEST(test_running_block_comes_back_paused_with_checkpoint_time);
  RUN_TEST(test_short_interruption_counts_the_time_that_passed);
  RUN_TEST(test_gap_limit_and_missing_or_backwards_clock);
  RUN_TEST(test_power_cut_alert_only_when_the_block_was_running);
  RUN_TEST(test_paused_block_keeps_its_time);
  RUN_TEST(test_restored_block_does_not_repeat_the_nudge_that_already_came);
  RUN_TEST(test_finished_block_leaves_nothing_to_restore);
  RUN_TEST(test_block_of_a_removed_subject_is_not_restored);
  RUN_TEST(test_requested_save_waits_until_things_are_quiet);
  RUN_TEST(test_save_now_writes_immediately);
  RUN_TEST(test_checkpoints_only_while_a_block_is_running);
  RUN_TEST(test_no_checkpoints_when_idle_or_without_clock);
  return UNITY_END();
}
