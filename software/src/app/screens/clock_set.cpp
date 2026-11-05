#include "app/screens/clock_set.h"
#include "app/navigation.h"
#include "ui/sound.h"
#include "drivers/display.h"
#include "drivers/rtc_clock.h"
#include "ui/colors.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using DateEdit::Field;

namespace {

constexpr int16_t kRowsY = 36;
constexpr int16_t kRowHeight = 25;
constexpr int16_t kLabelWidth = 190;
constexpr int16_t kValueWidth = 130;

}  // namespace

Field ClockSetScreen::fieldOfRow(uint8_t row) {
  switch (row) {
    case 0: return Field::Day;
    case 1: return Field::Month;
    case 2: return Field::Year;
    case 3: return Field::Hour;
    default: return Field::Minute;
  }
}

void ClockSetScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(0, 2, 320, 32, Font::Heading, Align::Left, Colors::kAccent, Colors::kBackground, 8);
  hint_.configure(0, 214, 320, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kRowCount; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    label_[r].configure(0, y, kLabelWidth, kRowHeight - 1, Font::Body, Align::Left, Colors::kText,
                        Colors::kBackground, 8);
    value_[r].configure(kLabelWidth, y, kValueWidth, kRowHeight - 1, Font::Body, Align::Right,
                        Colors::kMuted, Colors::kBackground, 8);
  }
  title_.set(Strings::kClockSetTitle);

  // Start from the time the clock shows now (or a plain default if the chip is missing).
  if (Rtc::isAvailable()) {
    DateTime now = Rtc::now();
    fields_ = {(uint16_t)now.year(), now.month(), now.day(), now.hour(), now.minute()};
    if (fields_.year < DateEdit::kMinYear) fields_.year = DateEdit::kMinYear;
    if (fields_.year > DateEdit::kMaxYear) fields_.year = DateEdit::kMaxYear;
  } else {
    fields_ = {2026, 1, 1, 12, 0};
  }

  selected_ = kFirstFieldRow;
  editing_ = false;
  dirty_ = true;
}

void ClockSetScreen::save() {
  Rtc::set(DateTime(fields_.year, fields_.month, fields_.day, fields_.hour, fields_.minute, 0));
}

void ClockSetScreen::onEvent(const Input::Event& event) {
  switch (event.type) {
    case Input::Event::Type::Turn:
      if (editing_) {
        DateEdit::change(fields_, fieldOfRow(selected_), event.delta);
      } else {
        int target = (int)selected_ + event.delta;
        if (target < 0 || target >= (int)kRowCount) break;
        selected_ = (uint8_t)target;
      }
      Sound::turn();
      dirty_ = true;
      break;

    case Input::Event::Type::Click:
      if (selected_ == kSaveRow) {
        if (Rtc::isAvailable()) save();
        Sound::pressLong();
        Nav::go(ScreenId::Settings);
        return;
      }
      if (selected_ == kCancelRow) {
        Sound::press();
        Nav::go(ScreenId::Settings);
        return;
      }
      Sound::press();
      editing_ = !editing_;
      dirty_ = true;
      break;

    case Input::Event::Type::LongPress:
      break;
  }
}

void ClockSetScreen::update() {
  if (!dirty_) return;
  drawRows();
  hint_.set(editing_ ? Strings::kSettingsEditHint : Strings::kSettingsHint);
  dirty_ = false;
}

void ClockSetScreen::drawRows() {
  const char* labels[kRowCount] = {Strings::kFieldDay, Strings::kFieldMonth, Strings::kFieldYear,
                                   Strings::kFieldHour, Strings::kFieldMinute, Strings::kSave,
                                   Strings::kCancel};
  for (uint8_t r = 0; r < kRowCount; r++) {
    bool selected = (r == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;

    char value[16] = "";
    switch (r) {
      case 0: snprintf(value, sizeof value, "%u.", (unsigned)fields_.day); break;
      case 1: snprintf(value, sizeof value, "%u.", (unsigned)fields_.month); break;
      case 2: snprintf(value, sizeof value, "%u.", (unsigned)fields_.year); break;
      case 3: snprintf(value, sizeof value, "%02u", (unsigned)fields_.hour); break;
      case 4: snprintf(value, sizeof value, "%02u", (unsigned)fields_.minute); break;
    }

    if (r == kCancelRow) label_[r].setIcon(Icons::Icon::BackSmall, Colors::kDim, TextField::IconSide::Before);
    label_[r].set(labels[r], Colors::kText, background);
    // While a value is being changed it is green, so she can see the dial now changes it.
    uint16_t valueColor = (selected && editing_) ? Colors::kGood : (selected ? Colors::kText : Colors::kMuted);
    value_[r].set(value, valueColor, background);
  }
}
