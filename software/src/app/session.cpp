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
