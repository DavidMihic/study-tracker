#include "app/screens/home.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "app/semester.h"
#include "app/subject_list.h"
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

void HomeScreen::enter() {
  Display::clear(Colors::kBackground);

  // Layout, landscape 320 x 240. Field = x, y, width, height. Below y = CardMenu::kBandTop is the menu.
  subject_.configure(10, 10, 300, 34, Font::Heading, Align::Center, Colors::kText);
  clock_.configure(10, 54, 300, 54, Font::Clock, Align::Center, Colors::kAccent);
  date_.configure(10, 114, 300, 26, Font::Body, Align::Center, Colors::kGood);
  notice_.configure(10, 142, 300, 22, Font::Small, Align::Center, Colors::kWarning);
  hint_.configure(10, 208, 300, 22, Font::Small, Align::Center, Colors::kMuted);

  menu_.close();
  drawBottom();
  drawSubject();
  redrawTime_ = true;
}

void HomeScreen::drawBottom() {
  notice_.invalidate();
  hint_.invalidate();
  hint_.set(Strings::kMenuHint);
  if (!Rtc::isAvailable()) {
    notice_.setColors(Colors::kBad, Colors::kBackground);
    notice_.set(Strings::kRtcMissing);
  } else if (Rtc::wasReset()) {
    notice_.set(Strings::kClockWasReset);
  }
}

int HomeScreen::selectedSubject() const {
  int last = Progress::lastSubjectIndex();
  if (last >= 0) return last;

  // First start (nothing remembered yet): the top subject of her current semester, if there is one.
  uint8_t semester = Progress::currentSemester();
  SubjectList::View view = {Semesters::yearOf(semester), Semesters::halfOf(semester)};
  size_t list[Subjects::kMaxSubjects];
  size_t n = SubjectList::buildMain(view, list, Subjects::kMaxSubjects);
  return n > 0 ? (int)list[0] : -1;
}

void HomeScreen::drawSubject() {
  int index = selectedSubject();
  if (index < 0) {
    subject_.setColors(Colors::kMuted, Colors::kBackground);
    subject_.set(Strings::kNoSubject);
  } else {
    subject_.setColors(Colors::kText, Colors::kBackground);
    subject_.set(Subjects::at(index).name);
  }
}

void HomeScreen::openMenu() {
  const CardMenu::Card cards[] = {
    {Icon::Play, Strings::kCardStart},
    {Icon::Book, Strings::kCardSubject},
    {Icon::Chart, Strings::kCardStats},
    {Icon::Gear, Strings::kCardSettings},
  };
  menu_.open(cards, 4, millis());
}

void HomeScreen::closeMenu() {
  menu_.close();
  drawBottom();   // the menu cleared the area where the notice and hint are
}

void HomeScreen::onEvent(const Input::Event& event) {
  if (!menu_.isOpen()) {
    if (event.type == Input::Event::Type::Click) {
      Sound::press();
      openMenu();
    }
    return;   // turns and long presses do nothing on the home screen
  }

  if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
  uint8_t choice = menu_.chosen();
  Sound::press();

  switch (choice) {
    case 0: {   // start studying
      int index = selectedSubject();
      if (index < 0) {                       // nothing to study yet: let her choose
        Nav::go(ScreenId::Picker);
        return;
      }
      if (!Rtc::isAvailable()) {             // sessions need the clock
        closeMenu();
        return;
      }
      Progress::setLastSubject(index);
      uint32_t target = (uint32_t)Progress::targetMinutes() * 60;
      AppState::session().startStudy(Rtc::now().unixtime(), (size_t)index, target);
      Persistence::saveNow();   // remembers the running block and the chosen subject
      Nav::go(ScreenId::Studying);
      return;
    }
    case 1:
      Nav::go(ScreenId::Picker);
      return;
    case 2:
      Nav::go(ScreenId::Stats);
      return;
    case 3:
      AppState::settingsReturn() = ScreenId::Home;
      Nav::go(ScreenId::Settings);
      return;
  }
}

void HomeScreen::update() {
  if (menu_.isOpen() && menu_.update(millis()) == CardMenu::Result::TimedOut) closeMenu();

  // Always call secondChanged() so it is consumed every loop.
  if (Rtc::secondChanged() || redrawTime_) {
    drawTime();
    redrawTime_ = false;
  }
}

void HomeScreen::drawTime() {
  char text[40];
  if (Rtc::isAvailable()) {
    DateTime now = Rtc::now();
    Format::clock(text, sizeof text, now.hour(), now.minute(), now.second());
    clock_.set(text);
    Format::date(text, sizeof text, now.dayOfTheWeek(), now.day(), now.month(), now.year());
    date_.set(text);
  } else {
    clock_.set("--:--:--");
  }
}
