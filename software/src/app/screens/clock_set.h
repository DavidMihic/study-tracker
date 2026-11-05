// Set the date and time with the dial. Turn moves through the rows; click on a row (Dan, Mjesec, Godina,
// Sat, Minute) lets the dial change its value, click again confirms it. "Spremi" writes the time to the
// clock chip (seconds start at 0), "Odustani" leaves without changing anything.
// Only reachable from the settings while no study block or break is running.
#pragma once
#include "app/date_edit.h"
#include "app/screens/screen.h"
#include "drivers/text_field.h"

class ClockSetScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  // Rows 0..4 are the date/time fields, then Spremi and Odustani. All rows fit on the screen, so there
  // is no Back row at the top (only menus that scroll have one).
  static constexpr uint8_t kFirstFieldRow = 0;
  static constexpr uint8_t kSaveRow = 5;
  static constexpr uint8_t kCancelRow = 6;
  static constexpr uint8_t kRowCount = 7;

  void drawRows();
  void save();
  static DateEdit::Field fieldOfRow(uint8_t row);

  DateEdit::Fields fields_ = {2026, 1, 1, 12, 0};
  TextField title_, hint_;
  TextField label_[kRowCount], value_[kRowCount];
  uint8_t selected_ = 0;
  bool editing_ = false;
  bool dirty_ = true;
};
