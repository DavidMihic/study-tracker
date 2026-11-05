// PC tests for saving to flash: the byte format (including damaged data) and the rules for WHEN and WHAT
// is written. The flash is replaced by an array in memory. Run with:  pio test -e native
#include <string.h>
#include <unity.h>
#include <vector>

#include "app/app_state.h"
#include "app/persistence.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "data/save_format.h"
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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_known_value);
  RUN_TEST(test_format_round_trip);
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
