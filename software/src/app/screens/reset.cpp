#include "app/screens/reset.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "data/progress.h"
#include "data/session_log.h"
#include "data/subjects.h"
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/sound.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using Icons::Icon;

namespace {

constexpr int16_t kRowsY = 40;
constexpr int16_t kRowHeight = 32;

const char* labelOf(uint8_t row) {
  switch (row) {
    case 0: return Strings::kResetOne;
    case 1: return Strings::kResetAll;
  }
  return Strings::kBack;
}

}  // namespace

void ResetScreen::enter() {
  // Coming back from the subject picker with a subject chosen: go straight to the confirmation.
  int chosen = AppState::pendingResetSubject();
  AppState::pendingResetSubject() = -1;
  if (chosen >= 0) showConfirm(chosen);
  else showMenu();
}

void ResetScreen::showMenu() {
  mode_ = Mode::Menu;
  menu_.close();
  Display::clear(Colors::kBackground);

  title_.configure(0, 4, 320, 32, Font::Heading, Align::Left, Colors::kAccent, Colors::kBackground, 8);
  hint_.configure(0, 208, 320, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kRowCount; r++) {
    label_[r].configure(0, kRowsY + r * kRowHeight, 320, kRowHeight - 2, Font::Body, Align::Left,
                        Colors::kText, Colors::kBackground, 8);
  }
  title_.set(Strings::kResetTitle);
  hint_.set(Strings::kYearHint);
  selected_ = kOne;
  dirty_ = true;
}

void ResetScreen::showConfirm(int subjectIndex) {
  mode_ = Mode::Confirm;
  target_ = subjectIndex;
  Display::clear(Colors::kBackground);

  question_.configure(0, 30, 320, 34, Font::Heading, Align::Center, Colors::kText);
  name_.configure(0, 74, 320, 30, Font::Body, Align::Center, Colors::kWarning);
  details_.configure(0, 112, 320, 24, Font::Small, Align::Center, Colors::kMuted);

  // What would be lost: time and number of sessions (of one subject, or the sum over all).
  uint32_t seconds = 0;
  unsigned sessions = 0;
  if (subjectIndex >= 0) {
    seconds = Progress::stats((size_t)subjectIndex).totalSeconds;
    sessions = Progress::stats((size_t)subjectIndex).sessions;
    question_.set(Strings::kResetQuestionOne);
    name_.set(Subjects::at((size_t)subjectIndex).name);
  } else {
    for (size_t i = 0; i < Subjects::count(); i++) {
      seconds += Progress::stats(i).totalSeconds;
      sessions += Progress::stats(i).sessions;
    }
    question_.set(Strings::kResetQuestionAll);
    name_.set("");
  }
  char duration[24], text[64];
  Format::duration(duration, sizeof duration, seconds);
  snprintf(text, sizeof text, Strings::kResetDetails, duration, sessions);
  details_.set(text);

  // Starts on Natrag, so a click by mistake changes nothing. No timeout: it waits for her.
  const CardMenu::Card cards[] = {
    {Icon::Back, Strings::kBack},
    {Icon::Cross, Strings::kCardReset},
  };
  menu_.open(cards, 2, millis(), 0, 0);
}

void ResetScreen::applyReset() {
  // The session log is cleared with the counters, so the statistics never disagree with the totals.
  if (target_ >= 0) {
    Progress::resetSubject((size_t)target_);
    SessionLog::removeSubject(Subjects::at((size_t)target_).id);
  } else {
    Progress::resetAllStats();
    SessionLog::clear();
  }
  Persistence::saveNow();     // a deliberate change: written right away
  Sound::pressLong();
}

void ResetScreen::onEvent(const Input::Event& event) {
  if (mode_ == Mode::Confirm) {
    if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
    if (menu_.chosen() == 1) applyReset();
    else Sound::press();
    showMenu();
    return;
  }

  switch (event.type) {
    case Input::Event::Type::Turn: {
      int target = (int)selected_ + event.delta;
      if (target < 0 || target >= (int)kRowCount) break;
      selected_ = (uint8_t)target;
      Sound::turn();
      dirty_ = true;
      break;
    }
    case Input::Event::Type::Click:
      Sound::press();
      if (selected_ == kOne) {
        AppState::pickerPurpose() = AppState::PickerPurpose::ResetOne;
        Nav::go(ScreenId::Picker);
      } else if (selected_ == kAll) {
        showConfirm(-1);
      } else {
        Nav::go(ScreenId::Settings);
      }
      return;
    case Input::Event::Type::LongPress:
      break;
  }
}

void ResetScreen::update() {
  if (mode_ == Mode::Confirm || !dirty_) return;
  drawRows();
  dirty_ = false;
}

void ResetScreen::drawRows() {
  for (uint8_t r = 0; r < kRowCount; r++) {
    uint16_t background = (r == selected_) ? Colors::kSelection : Colors::kBackground;
    if (r == kBack) label_[r].setIcon(Icon::BackSmall, Colors::kDim, TextField::IconSide::Before);
    else label_[r].setIcon(Icon::Return, Colors::kDim, TextField::IconSide::Before);
    label_[r].set(labelOf(r), Colors::kText, background);
  }
}
