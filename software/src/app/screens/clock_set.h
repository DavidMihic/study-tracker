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
  // Row 0 is Odustani (so leaving needs no long scroll), rows 1..5 are the date/time fields, then
  // Spremi and Odustani again.
  static constexpr uint8_t kCancelTopRow = 0;
  static constexpr uint8_t kFirstFieldRow = 1;
  static constexpr uint8_t kSaveRow = 6;
  static constexpr uint8_t kCancelRow = 7;
  static constexpr uint8_t kRowCount = 8;

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
