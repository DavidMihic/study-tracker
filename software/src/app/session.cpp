#include "app/session.h"
#include "config/study_rules.h"

void Session::startStudy(uint32_t nowUnix, size_t subjectIndex, uint32_t targetSeconds) {
  phase_ = Phase::Studying;
  subjectIndex_ = subjectIndex;
  accumulated_ = 0;
  runStart_ = nowUnix;
  running_ = true;
  targetSeconds_ = targetSeconds;
  nudgeFired_ = false;
  checkpoint_ = nowUnix;
}

void Session::pause(uint32_t nowUnix) {
  if (phase_ != Phase::Studying || !running_) return;
  accumulated_ = elapsedSeconds(nowUnix);
  running_ = false;
}

void Session::resume(uint32_t nowUnix) {
  if (phase_ != Phase::Studying || running_) return;
  runStart_ = nowUnix;
  running_ = true;
  checkpoint_ = nowUnix;
}

uint32_t Session::elapsedSeconds(uint32_t nowUnix) const {
  if (phase_ == Phase::Idle) return 0;
  if (!running_ || nowUnix < runStart_) return accumulated_;   // paused, or clock set backwards: no wrap
  return accumulated_ + (nowUnix - runStart_);
}

bool Session::targetReached(uint32_t nowUnix) const {
  return phase_ == Phase::Studying && elapsedSeconds(nowUnix) >= targetSeconds_;
}

bool Session::nudgeDue(uint32_t nowUnix) {
  if (nudgeFired_ || !targetReached(nowUnix)) return false;
  nudgeFired_ = true;
  return true;
}

uint32_t Session::endStudy(uint32_t nowUnix) {
  if (phase_ != Phase::Studying) return 0;
  uint32_t length = elapsedSeconds(nowUnix);
  phase_ = Phase::Break;
  accumulated_ = 0;
  runStart_ = nowUnix;
  running_ = true;
  return length;
}

bool Session::isCounted(uint32_t seconds) {
  return seconds >= StudyRules::kMinCountedSessionSeconds;
}

void Session::endBreak() {
  phase_ = Phase::Idle;
  running_ = false;
}

void Session::checkpoint(uint32_t nowUnix) {
  if (phase_ == Phase::Studying && running_ && nowUnix >= runStart_) checkpoint_ = nowUnix;
}

Session::Snapshot Session::snapshot() const {
  Snapshot s = {};
  s.active = (phase_ == Phase::Studying);
  s.subjectIndex = subjectIndex_;
  s.accumulatedSeconds = accumulated_;
  s.runStartUnix = runStart_;
  s.running = running_;
  s.checkpointUnix = checkpoint_;
  return s;
}

void Session::restorePaused(const Snapshot& s, uint32_t targetSeconds, uint32_t nowUnix) {
  phase_ = Phase::Studying;
  subjectIndex_ = s.subjectIndex;
  accumulated_ = s.accumulatedSeconds;
  if (s.running) {
    // The time until the last checkpoint counts for sure; what happened after it is unknown. A short
    // gap is taken as "kept running"; a long one (or a clock that is missing or went backwards) is not.
    uint32_t until = s.checkpointUnix;
    bool shortGap = nowUnix >= s.checkpointUnix && nowUnix - s.checkpointUnix <= StudyRules::kResumeTrustSeconds;
    if (nowUnix != 0 && shortGap) until = nowUnix;
    if (until > s.runStartUnix) accumulated_ += until - s.runStartUnix;
  }
  runStart_ = s.checkpointUnix;
  running_ = false;
  targetSeconds_ = targetSeconds;
  checkpoint_ = s.checkpointUnix;
  nudgeFired_ = (accumulated_ >= targetSeconds_);   // a nudge that was already due is not repeated
}
