#include "app/persistence.h"
#include "app/app_state.h"
#include "data/progress.h"
#include "config/study_rules.h"
#include "data/save_format.h"
#include "data/session_log.h"
#include "data/subjects.h"

namespace Persistence {

namespace {

Backend* backend = nullptr;
Clock timeSource = {nullptr, nullptr};

SaveFormat::Data data;                       // working copy, static: it is a few kilobytes
uint8_t buffer[SaveFormat::kMaxBytes];

bool saveRequested = false;
uint32_t lastRequestMs = 0;
uint32_t lastCheckpointUnix = 0;
bool restored = false;
bool restoredRunning = false;
bool autoFinished = false;                   // restoreSession() recorded an old block by itself
bool hasPending = false;                     // load() found a running block; restoreSession() applies it
Session::Snapshot pending = {};

uint32_t nowMs() { return timeSource.nowMs ? timeSource.nowMs() : 0; }
uint32_t nowUnix() { return timeSource.nowUnix ? timeSource.nowUnix() : 0; }

// Copies the current state into `data`.
void collect() {
  data.semester = Progress::currentSemester();
  data.targetMinutes = Progress::targetMinutes();
  data.brightness = Progress::brightness();
  data.soundTurn = Progress::soundTurn();
  data.soundPress = Progress::soundPress();
  data.soundNudge = Progress::soundNudge();
  data.soundRestore = Progress::soundRestore();

  int last = Progress::lastSubjectIndex();
  data.lastSubjectId = (last >= 0) ? Subjects::at((size_t)last).id : 0;

  // Only subjects with something to remember take space.
  data.subjectCount = 0;
  for (size_t i = 0; i < Subjects::count(); i++) {
    const SubjectStats& s = Progress::stats(i);
    if (s.totalSeconds == 0 && s.sessions == 0 && !s.favorite) continue;
    data.subjects[data.subjectCount++] = {Subjects::at(i).id, s.totalSeconds, s.sessions, s.favorite};
  }

  Session::Snapshot snap = AppState::session().snapshot();
  data.active = {};
  if (snap.active) {
    data.active.active = true;
    data.active.subjectId = Subjects::at(snap.subjectIndex).id;
    data.active.accumulatedSeconds = snap.accumulatedSeconds;
    data.active.runStartUnix = snap.runStartUnix;
    data.active.running = snap.running;
    data.active.checkpointUnix = snap.checkpointUnix;
    data.active.startedUnix = snap.startedUnix;
  }
}

}  // namespace

void begin(Backend* b, Clock c) {
  backend = b;
  timeSource = c;
  saveRequested = false;
  restored = false;
  restoredRunning = false;
  autoFinished = false;
  hasPending = false;
}

bool load() {
  restored = false;
  hasPending = false;
  if (!backend) return false;

  size_t length = backend->read(buffer, sizeof buffer);
  if (length == 0 || !SaveFormat::parse(buffer, length, data)) return false;

  Progress::setCurrentSemester(data.semester);
  Progress::setTargetMinutes(data.targetMinutes);
  Progress::setBrightness(data.brightness);
  Progress::setSoundTurn(data.soundTurn);
  Progress::setSoundPress(data.soundPress);
  Progress::setSoundNudge(data.soundNudge);
  Progress::setSoundRestore(data.soundRestore);

  // Records of subjects that are no longer in the list are skipped (their data is dropped at the next save).
  for (uint16_t i = 0; i < data.subjectCount; i++) {
    int index = Subjects::indexOfId(data.subjects[i].id);
    if (index < 0) continue;
    Progress::loadStats((size_t)index, data.subjects[i].totalSeconds, data.subjects[i].sessions,
                        data.subjects[i].favorite);
  }
  int last = Subjects::indexOfId(data.lastSubjectId);
  if (data.lastSubjectId != 0 && last >= 0) Progress::setLastSubject((size_t)last);

  if (data.active.active) {
    int index = Subjects::indexOfId(data.active.subjectId);
    if (index >= 0) {
      pending = {};
      pending.active = true;
      pending.subjectIndex = (size_t)index;
      pending.accumulatedSeconds = data.active.accumulatedSeconds;
      pending.runStartUnix = data.active.runStartUnix;
      pending.running = data.active.running;
      pending.checkpointUnix = data.active.checkpointUnix;
      pending.startedUnix = data.active.startedUnix;
      hasPending = true;
    }
  }
  lastCheckpointUnix = nowUnix();
  return true;
}

void restoreSession() {
  if (!hasPending) return;
  hasPending = false;
  Session& session = AppState::session();
  const uint32_t now = nowUnix();
  session.restorePaused(pending, (uint32_t)Progress::targetMinutes() * 60, now);
  lastCheckpointUnix = now;

  // Waiting for hours (clock must work and not have gone backwards): finish the block for her.
  const bool old = now != 0 && now >= pending.checkpointUnix &&
                   now - pending.checkpointUnix > StudyRules::kAutoFinishSeconds;
  if (old) {
    uint32_t length = session.endStudy(now);
    session.endBreak();                      // back to idle: the device starts on the home screen
    if (Session::isCounted(length)) {
      Progress::recordSession(pending.subjectIndex, length);
      SessionLog::append({pending.startedUnix, length, Subjects::at(pending.subjectIndex).id});
      autoFinished = true;
    }
    saveNow();                               // the total is saved and the running block is cleared
    return;
  }

  restored = true;
  restoredRunning = pending.running;
}

void saveNow() {
  if (!backend) return;
  AppState::session().checkpoint(nowUnix());
  collect();
  size_t length = SaveFormat::serialize(data, buffer, sizeof buffer);
  if (length > 0) backend->write(buffer, length);
  saveRequested = false;
  lastCheckpointUnix = nowUnix();
}

void requestSave() {
  saveRequested = true;
  lastRequestMs = nowMs();
}

void update() {
  if (!backend) return;

  if (saveRequested && nowMs() - lastRequestMs >= kSaveDelayMs) {
    saveNow();
    return;
  }

  // While a block is running: a checkpoint every few minutes (only if the clock works).
  const Session& session = AppState::session();
  uint32_t now = nowUnix();
  if (now != 0 && session.phase() == Session::Phase::Studying && !session.isPaused() &&
      now >= lastCheckpointUnix && now - lastCheckpointUnix >= kCheckpointSeconds) {
    saveNow();
  }
}

bool restoredSession() { return restored; }

bool restoredWasRunning() { return restored && restoredRunning; }

void acknowledgeRestored() { restored = false; restoredRunning = false; }

bool blockAutoFinished() { return autoFinished; }

void acknowledgeAutoFinished() { autoFinished = false; }

}  // namespace Persistence
