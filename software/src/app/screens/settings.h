// Settings: brightness, current semester, length of a study block. Turn scrolls through the rows,
// click on a row lets you change its value with the dial (click again = done), "Natrag" goes back to
// the screen that opened the settings. A running study session keeps counting while she is here.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"

class SettingsScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum Row : uint8_t { kBrightness, kSemester, kTarget, kClock, kBack, kRowCount };

  void changeValue(int delta);
  void drawRows();
  void valueText(uint8_t row, char* out, size_t size) const;

  TextField title_, hint_;
  TextField label_[kRowCount], value_[kRowCount];
  uint8_t selected_ = 0;
  bool editing_ = false;
  bool dirty_ = true;
};
