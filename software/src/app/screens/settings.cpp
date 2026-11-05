#include "app/screens/settings.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "drivers/backlight.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;

namespace {

constexpr int16_t kRowsY = 40;
constexpr int16_t kRowHeight = 32;
constexpr int16_t kLabelWidth = 180;
constexpr int16_t kValueWidth = 140;

bool clockCanBeSet() { return AppState::session().phase() == Session::Phase::Idle; }

const char* labelOf(uint8_t row) {
  switch (row) {
    case 0: return Strings::kSettingBrightness;
    case 1: return Strings::kSettingSemester;
    case 2: return Strings::kSettingTarget;
    case 3: return Strings::kSettingClock;
  }
  return Strings::kBack;
}

}  // namespace

void SettingsScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(0, 4, 320, 32, Font::Heading, Align::Left, Colors::kAccent, Colors::kBackground, 8);
  hint_.configure(0, 208, 320, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kRowCount; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    label_[r].configure(0, y, kLabelWidth, kRowHeight - 2, Font::Body, Align::Left, Colors::kText,
                        Colors::kBackground, 8);
    value_[r].configure(kLabelWidth, y, kValueWidth, kRowHeight - 2, Font::Body, Align::Right,
                        Colors::kMuted, Colors::kBackground, 8);
  }
  title_.set(Strings::kSettingsTitle);

  selected_ = 0;
  editing_ = false;
  dirty_ = true;
}

void SettingsScreen::valueText(uint8_t row, char* out, size_t size) const {
  out[0] = '\0';
  switch (row) {
    case kBrightness:
      snprintf(out, size, Strings::kBrightnessFormat, (unsigned)(Backlight::get() * 100 / 255));
      break;
    case kSemester:
      snprintf(out, size, Strings::kSemesterFormat, (unsigned)Progress::currentSemester());
      break;
    case kTarget:
      snprintf(out, size, "%u %s", (unsigned)Progress::targetMinutes(), Strings::kMinutes);
      break;
    case kClock:   // the time must not jump under a running block, so it can only be set when idle
      snprintf(out, size, "%s", clockCanBeSet() ? ">" : Strings::kClockLocked);
      break;
  }
}

void SettingsScreen::changeValue(int delta) {
  switch (selected_) {
    case kBrightness:
      Backlight::adjust(delta);
      break;
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

void SettingsScreen::onEvent(const Input::Event& event) {
  switch (event.type) {
    case Input::Event::Type::Turn:
      if (editing_) {
        changeValue(event.delta);
        Buzzer::play(Buzzer::Pattern::Click);
      } else {
        int target = (int)selected_ + event.delta;
        if (target < 0 || target >= (int)kRowCount) break;
        selected_ = (uint8_t)target;
        Buzzer::play(Buzzer::Pattern::Click);
      }
      dirty_ = true;
      break;

    case Input::Event::Type::Click:
      Buzzer::play(Buzzer::Pattern::Short);
      if (selected_ == kBack) {
        Nav::go(AppState::settingsReturn());
        return;
      }
      if (selected_ == kClock) {
        if (clockCanBeSet()) Nav::go(ScreenId::ClockSet);
        break;
      }
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
  for (uint8_t r = 0; r < kRowCount; r++) {
    bool selected = (r == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;
    char value[24];
    valueText(r, value, sizeof value);

    char label[32];
    snprintf(label, sizeof label, "%s%s", r == kBack ? "< " : "", labelOf(r));
    label_[r].set(label, Colors::kText, background);
    // While a value is being changed it is highlighted, so she can see the dial now changes it.
    uint16_t valueColor = (selected && editing_) ? Colors::kGood : (selected ? Colors::kText : Colors::kMuted);
    value_[r].set(value, valueColor, background);
  }
}
