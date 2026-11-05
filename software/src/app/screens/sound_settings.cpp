#include "app/screens/sound_settings.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "data/progress.h"
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
constexpr int16_t kLabelWidth = 200;
constexpr int16_t kValueWidth = 120;

const char* labelOf(uint8_t row) {
  switch (row) {
    case 0: return Strings::kSettingSoundTurn;
    case 1: return Strings::kSettingSoundPress;
    case 2: return Strings::kSettingSoundNudge;
    case 3: return Strings::kSettingSoundRestore;
  }
  return Strings::kBack;
}

}  // namespace

void SoundSettingsScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(0, 4, 292, 32, Font::Heading, Align::Left, Colors::kAccent, Colors::kBackground, 8);
  hint_.configure(0, 208, 292, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kRowCount; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    label_[r].configure(0, y, kLabelWidth, kRowHeight - 2, Font::Body, Align::Left, Colors::kText,
                        Colors::kBackground, 8);
    value_[r].configure(kLabelWidth, y, kValueWidth, kRowHeight - 2, Font::Body, Align::Right,
                        Colors::kMuted, Colors::kBackground, 8);
  }
  title_.set(Strings::kSoundTitle);
  hint_.set(Strings::kSoundHint);

  selected_ = kTurn;
  dirty_ = true;
}

bool SoundSettingsScreen::isOn(uint8_t row) const {
  switch (row) {
    case kTurn:  return Progress::soundTurn();
    case kPress: return Progress::soundPress();
    case kNudge: return Progress::soundNudge();
    default:     return Progress::soundRestore();
  }
}

void SoundSettingsScreen::onEvent(const Input::Event& event) {
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
      switch (selected_) {
        case kTurn:  Progress::setSoundTurn(!Progress::soundTurn());    Sound::turn();  break;
        case kPress: Progress::setSoundPress(!Progress::soundPress());  Sound::press(); break;
        case kNudge: Progress::setSoundNudge(!Progress::soundNudge());  Sound::nudge(); break;
        case kRestore: Progress::setSoundRestore(!Progress::soundRestore());  Sound::powerRestored(); break;
        case kBack:
          Sound::press();
          Nav::go(ScreenId::Settings);
          return;
      }
      Persistence::requestSave();
      dirty_ = true;
      break;

    case Input::Event::Type::LongPress:
      break;
  }
}

void SoundSettingsScreen::update() {
  if (!dirty_) return;
  drawRows();
  dirty_ = false;
}

void SoundSettingsScreen::drawRows() {
  for (uint8_t r = 0; r < kRowCount; r++) {
    bool selected = (r == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;

    if (r == kBack) label_[r].setIcon(Icon::BackSmall, Colors::kDim, TextField::IconSide::Before);
    label_[r].set(labelOf(r), Colors::kText, background);

    if (r == kBack) {
      value_[r].set("", Colors::kMuted, background);
    } else {
      bool on = isOn(r);
      uint16_t color = on ? Colors::kGood : Colors::kMuted;
      value_[r].set(on ? Strings::kOn : Strings::kOff, color, background);
    }
  }
}
