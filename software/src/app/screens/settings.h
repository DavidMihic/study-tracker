// Settings: brightness, current semester, length of a study block, and the two submenus (sound, date and
// time). Turn scrolls through the rows. Click on a value row lets you change it with the dial (click
// again = done); click on a submenu row opens it. "Natrag" goes back to the screen that opened the
// settings. A running study session keeps counting while she is here.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"
#include "ui/scroll_arrows.h"

class SettingsScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum Row : uint8_t { kBackTop, kBrightness, kSemester, kTarget, kSound, kClock, kBack, kRowCount };
  static constexpr uint8_t kVisibleRows = 5;

  void changeValue(int delta);
  void keepSelectionVisible();
  void drawRows();
  void valueText(uint8_t row, char* out, size_t size) const;

  TextField title_, hint_;
  TextField label_[kVisibleRows], value_[kVisibleRows];
  ScrollArrows arrows_;
  uint8_t selected_ = 0;
  uint8_t top_ = 0;              // first visible row
  bool editing_ = false;
  bool dirty_ = true;
  bool comingFromSubmenu_ = false;   // true while a submenu is open: coming back keeps the position
};
