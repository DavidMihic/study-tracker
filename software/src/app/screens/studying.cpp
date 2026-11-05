#include "app/screens/studying.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "data/progress.h"
#include "data/subjects.h"
#include "ui/sound.h"
#include "drivers/display.h"
#include "drivers/rtc_clock.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using Icons::Icon;

namespace {
constexpr uint32_t kAlertRepeatMs = 20000;   // the power-cut alert repeats until she touches the device
}

void StudyingScreen::enter() {
  Display::clear(Colors::kBackground);

  subject_.configure(10, 14, 300, 34, Font::Heading, Align::Center, Colors::kText);
  timer_.configure(10, 70, 300, 54, Font::Clock, Align::Center, Colors::kAccent);
  target_.configure(10, 134, 300, 26, Font::Body, Align::Center, Colors::kMuted);
  hint_.configure(10, 208, 300, 22, Font::Small, Align::Center, Colors::kMuted);

  menu_.close();
  subject_.set(Subjects::at(AppState::session().subjectIndex()).name);
  if (Persistence::restoredSession()) {     // opened right after a power cut
    alertActive_ = Persistence::restoredWasRunning();
    nextAlertMs_ = millis();
    hint_.set(Strings::kRestoredNotice);
    Persistence::acknowledgeRestored();
  } else {
    hint_.set(Strings::kMenuHint);
  }

  statusDrawn_ = false;
  redraw_ = true;
}

void StudyingScreen::openMenu() {
  const bool paused = AppState::session().isPaused();
  const CardMenu::Card cards[] = {
    paused ? CardMenu::Card{Icon::Play, Strings::kCardResume} : CardMenu::Card{Icon::Pause, Strings::kCardPause},
    {Icon::Stop, Strings::kCardStop},
    {Icon::Gear, Strings::kCardSettings},
  };
  menu_.open(cards, 3, millis());
}

void StudyingScreen::closeMenu() {
  menu_.close();
  hint_.invalidate();
  hint_.set(Strings::kMenuHint);
}

void StudyingScreen::stopStudying() {
  Session& session = AppState::session();
  uint32_t length = session.endStudy(Rtc::now().unixtime());

  AppState::LastBlock& last = AppState::lastBlock();
  last.seconds = length;
  last.counted = Session::isCounted(length);
  if (last.counted) Progress::recordSession(session.subjectIndex(), length);
  Persistence::saveNow();   // the new total is saved and the running block is cleared

  Sound::pressLong();
  Nav::go(ScreenId::Break);
}

void StudyingScreen::onEvent(const Input::Event& event) {
  alertActive_ = false;   // any touch means she has seen it

  // With the menu closed only a click does anything, and it only opens the menu, so a stray turn or
  // press can never change the session.
  if (!menu_.isOpen()) {
    if (event.type == Input::Event::Type::Click) {
      Sound::press();
      openMenu();
    }
    return;
  }

  if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
  Session& session = AppState::session();
  Sound::press();

  switch (menu_.chosen()) {
    case 0:   // pause / resume
      if (session.isPaused()) session.resume(Rtc::now().unixtime());
      else session.pause(Rtc::now().unixtime());
      Persistence::saveNow();
      closeMenu();
      redraw_ = true;
      break;
    case 1:
      stopStudying();
      break;
    case 2:
      AppState::settingsReturn() = ScreenId::Studying;
      Nav::go(ScreenId::Settings);
      break;
  }
}

void StudyingScreen::update() {
  Session& session = AppState::session();
  if (menu_.isOpen() && menu_.update(millis()) == CardMenu::Result::TimedOut) closeMenu();

  if (alertActive_ && (int32_t)(millis() - nextAlertMs_) >= 0) {
    Sound::powerRestored();                 // silent if she switched this alert off in the settings
    nextAlertMs_ = millis() + kAlertRepeatMs;
  }

  bool tick = Rtc::secondChanged();
  if (tick || redraw_) {
    uint32_t now = Rtc::now().unixtime();

    // The nudge: once, when the target is reached. The timer keeps running.
    if (session.nudgeDue(now)) Sound::nudge();

    Shown state = session.isPaused() ? Shown::Paused
                : session.targetReached(now) ? Shown::Reached : Shown::Running;
    if (!statusDrawn_ || state != shown_) drawStatus(state);
    drawTimer();
    redraw_ = false;
  }
}

void StudyingScreen::drawStatus(Shown state) {
  shown_ = state;
  statusDrawn_ = true;
  char text[40];
  switch (state) {
    case Shown::Paused:
      timer_.setColors(Colors::kWarning, Colors::kBackground);
      target_.set(Strings::kPaused, Colors::kWarning, Colors::kBackground);
      break;
    case Shown::Reached:
      timer_.setColors(Colors::kGood, Colors::kBackground);
      target_.set(Strings::kTargetReached, Colors::kGood, Colors::kBackground);
      break;
    case Shown::Running:
      snprintf(text, sizeof text, Strings::kTargetFormat, (unsigned)(AppState::session().targetSeconds() / 60));
      timer_.setColors(Colors::kAccent, Colors::kBackground);
      target_.set(text, Colors::kMuted, Colors::kBackground);
      break;
  }
}

void StudyingScreen::drawTimer() {
  char text[16];
  Format::elapsed(text, sizeof text, AppState::session().elapsedSeconds(Rtc::now().unixtime()));
  timer_.set(text);
}
