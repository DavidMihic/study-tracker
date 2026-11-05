// Sound submenu of the settings: three switches (dial ticks, button beeps, nudge beeps). Click on a
// switch flips it and plays that sound as a preview (silent if it is now off). "Natrag" returns to
// the settings.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"

class SoundSettingsScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum Row : uint8_t { kBackTop, kTurn, kPress, kNudge, kBack, kRowCount };

  bool isOn(uint8_t row) const;
  void drawRows();

  TextField title_, hint_;
  TextField label_[kRowCount], value_[kRowCount];
  uint8_t selected_ = 0;
  bool dirty_ = true;
};
