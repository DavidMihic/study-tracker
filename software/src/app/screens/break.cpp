#include "app/screens/break.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "data/progress.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "drivers/rtc_clock.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using Icons::Icon;

void BreakScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(10, 14, 300, 34, Font::Heading, Align::Center, Colors::kText);
  timer_.configure(10, 70, 300, 54, Font::Clock, Align::Center, Colors::kAccent);
  lastBlock_.configure(10, 134, 300, 26, Font::Body, Align::Center, Colors::kGood);
  hint_.configure(10, 208, 300, 22, Font::Small, Align::Center, Colors::kMuted);

  menu_.close();
  title_.set(Strings::kBreak);
  hint_.set(Strings::kMenuHint);

  const AppState::LastBlock& last = AppState::lastBlock();
  char text[48];
  if (last.counted) {
    char duration[24];
    Format::duration(duration, sizeof duration, last.seconds);
    snprintf(text, sizeof text, Strings::kLastBlockFormat, duration);
    lastBlock_.set(text, Colors::kGood, Colors::kBackground);
  } else {
    lastBlock_.set(Strings::kBlockNotCounted, Colors::kMuted, Colors::kBackground);
  }
  redraw_ = true;
}

void BreakScreen::closeMenu() {
  menu_.close();
  hint_.invalidate();
  hint_.set(Strings::kMenuHint);
}

void BreakScreen::onEvent(const Input::Event& event) {
  Session& session = AppState::session();

  if (!menu_.isOpen()) {
    if (event.type == Input::Event::Type::Click) {
      Buzzer::play(Buzzer::Pattern::Short);
      const CardMenu::Card cards[] = {
        {Icon::Play, Strings::kCardNewBlock},
        {Icon::Home, Strings::kCardHome},
      };
      menu_.open(cards, 2, millis());
    }
    return;
  }

  if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
  Buzzer::play(Buzzer::Pattern::Short);

  if (menu_.chosen() == 0) {   // another block on the same subject
    uint32_t target = (uint32_t)Progress::targetMinutes() * 60;
    size_t subject = session.subjectIndex();
    session.endBreak();
    session.startStudy(Rtc::now().unixtime(), subject, target);
    Nav::go(ScreenId::Studying);
  } else {
    session.endBreak();
    Nav::go(ScreenId::Home);
  }
}

void BreakScreen::update() {
  if (menu_.isOpen() && menu_.update(millis()) == CardMenu::Result::TimedOut) closeMenu();

  if (Rtc::secondChanged() || redraw_) {
    char text[16];
    Format::elapsed(text, sizeof text, AppState::session().elapsedSeconds(Rtc::now().unixtime()));
    timer_.set(text);
    redraw_ = false;
  }
}
