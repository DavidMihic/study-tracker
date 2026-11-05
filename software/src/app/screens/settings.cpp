#include "app/screens/settings.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "drivers/backlight.h"
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/sound.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using Icons::Icon;

namespace {

constexpr int16_t kRowsY = 40;
constexpr int16_t kRowHeight = 32;
constexpr int16_t kScreenWidth = 320;
constexpr int16_t kLabelWidth = 200;
constexpr int16_t kValueWidth = kScreenWidth - kLabelWidth;
constexpr int16_t kTextWidth = kScreenWidth - ScrollArrows::kReservedWidth;   // title and hint leave the corners free

constexpr int kBrightnessStepPercent = 5;

bool clockCanBeSet() { return AppState::session().phase() == Session::Phase::Idle; }

const char* labelOf(uint8_t row) {
  switch (row) {
    case 1: return Strings::kSettingSemester;
    case 2: return Strings::kSettingTarget;
    case 3: return Strings::kSettingBrightness;
    case 4: return Strings::kSettingSound;
    case 5: return Strings::kSettingClock;
    case 6: return Strings::kSettingReset;
  }
  return Strings::kBack;   // rows 0 and 7
}

// Brightness is shown in whole percent and moves in steps of 5 %. The backlight itself uses 0..255,
// so the percent is rounded to the nearest multiple of 5 for display.
int brightnessPercent() {
  int exact = Backlight::get() * 100 / 255;
  return ((exact + kBrightnessStepPercent / 2) / kBrightnessStepPercent) * kBrightnessStepPercent;
}

}  // namespace

void SettingsScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(0, 4, kTextWidth, 32, Font::Heading, Align::Left, Colors::kAccent, Colors::kBackground, 8);
  hint_.configure(0, 208, kTextWidth, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kVisibleRows; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    label_[r].configure(0, y, kLabelWidth, kRowHeight - 2, Font::Body, Align::Left, Colors::kText,
                        Colors::kBackground, 8);
    value_[r].configure(kLabelWidth, y, kValueWidth, kRowHeight - 2, Font::Body, Align::Right,
                        Colors::kMuted, Colors::kBackground, 8);
  }
  title_.set(Strings::kSettingsTitle);
  arrows_.reset();

  // Coming back from a submenu keeps the row she was on; a fresh visit starts on the first setting
  // (row 0 is "Natrag", there so that leaving needs no long scroll).
  if (!comingFromSubmenu_) {
    selected_ = kSemester;
    top_ = 0;
  }
  comingFromSubmenu_ = false;
  editing_ = false;
  dirty_ = true;
}

void SettingsScreen::valueText(uint8_t row, char* out, size_t size) const {
  out[0] = '\0';
  switch (row) {
    case kBrightness:
      snprintf(out, size, Strings::kBrightnessFormat, (unsigned)brightnessPercent());
      break;
    case kSemester:
      snprintf(out, size, Strings::kSemesterFormat, (unsigned)Progress::currentSemester());
      break;
    case kTarget:
      snprintf(out, size, "%u %s", (unsigned)Progress::targetMinutes(), Strings::kMinutes);
      break;
    case kClock:   // the time must not jump under a running block, so it can only be set when idle
      snprintf(out, size, "%s", clockCanBeSet() ? "" : Strings::kClockLocked);
      break;
  }
}

void SettingsScreen::changeValue(int delta) {
  switch (selected_) {
    case kBrightness: {
      int percent = brightnessPercent() + delta * kBrightnessStepPercent;
      if (percent < kBrightnessStepPercent) percent = kBrightnessStepPercent;   // never fully dark
      if (percent > 100) percent = 100;
      Backlight::set(percent * 255 / 100);
      Progress::setBrightness(Backlight::get());
      break;
    }
    case kSemester:
      Progress::setCurrentSemester((uint8_t)(Progress::currentSemester() + delta));
      break;
    case kTarget: {
      int minutes = (int)Progress::targetMinutes() + delta * StudyRules::kTargetStepMinutes;
      if (minutes < 0) minutes = 0;   // the setter clamps; this only avoids wrapping below zero
      Progress::setTargetMinutes((uint16_t)minutes);
      // A block that is running now follows the new target (a nudge that already came is not repeated).
      Session& session = AppState::session();
      if (session.phase() == Session::Phase::Studying) {
        session.setTargetSeconds((uint32_t)Progress::targetMinutes() * 60);
      }
      break;
    }
  }
}

void SettingsScreen::keepSelectionVisible() {
  if (selected_ < top_) top_ = selected_;
  if (selected_ >= top_ + kVisibleRows) top_ = selected_ - kVisibleRows + 1;
}

void SettingsScreen::onEvent(const Input::Event& event) {
  switch (event.type) {
    case Input::Event::Type::Turn:
      if (editing_) {
        changeValue(event.delta);
        Persistence::requestSave();   // waits until the dial stops moving, so a spin is one write
      } else {
        int target = (int)selected_ + event.delta;
        if (target < 0 || target >= (int)kRowCount) break;
        selected_ = (uint8_t)target;
        keepSelectionVisible();
      }
      Sound::turn();
      dirty_ = true;
      break;

    case Input::Event::Type::Click:
      if (selected_ == kBack || selected_ == kBackTop) {
        Sound::press();
        Nav::go(AppState::settingsReturn());
        return;
      }
      if (selected_ == kSound) {
        Sound::press();
        comingFromSubmenu_ = true;
        Nav::go(ScreenId::SoundSettings);
        return;
      }
      if (selected_ == kReset) {
        Sound::press();
        comingFromSubmenu_ = true;
        Nav::go(ScreenId::Reset);
        return;
      }
      if (selected_ == kClock) {
        if (clockCanBeSet()) {
          Sound::press();
          comingFromSubmenu_ = true;
          Nav::go(ScreenId::ClockSet);
          return;
        }
        break;
      }
      Sound::press();
      editing_ = !editing_;
      dirty_ = true;
      break;

    case Input::Event::Type::LongPress:
      break;
  }
}

void SettingsScreen::update() {
  if (!dirty_) return;
  drawRows();
  hint_.set(editing_ ? Strings::kSettingsEditHint : Strings::kSettingsHint);
  dirty_ = false;
}

void SettingsScreen::drawRows() {
  for (uint8_t r = 0; r < kVisibleRows; r++) {
    uint8_t row = top_ + r;
    if (row >= kRowCount) {
      label_[r].clearIcon();
      label_[r].set("", Colors::kText, Colors::kBackground);
      value_[r].set("", Colors::kMuted, Colors::kBackground);
      continue;
    }
    bool selected = (row == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;
    char value[24];
    valueText(row, value, sizeof value);

    // Rows that open another screen start with a dimmed icon: a return arrow, or a left triangle for Back.
    bool submenu = (row == kSound) || (row == kReset) || (row == kClock && clockCanBeSet());
    if (submenu) label_[r].setIcon(Icon::Return, Colors::kDim, TextField::IconSide::Before);
    else if (row == kBack || row == kBackTop) label_[r].setIcon(Icon::BackSmall, Colors::kDim, TextField::IconSide::Before);
    else label_[r].clearIcon();
    label_[r].set(labelOf(row), Colors::kText, background);

    // While a value is being changed it is highlighted, so she can see the dial now changes it.
    uint16_t valueColor = (selected && editing_) ? Colors::kGood : (selected ? Colors::kText : Colors::kMuted);
    value_[r].set(value, valueColor, background);
  }
  arrows_.update(top_ > 0, top_ + kVisibleRows < kRowCount);
}
