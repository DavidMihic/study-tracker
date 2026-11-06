// Statistics screen. Pages, in the order the dial turns through them:
//   0  Today: total, and the subjects studied today (most time first)
//   1  This week: bar chart Monday to Sunday, total, most studied subject
//   2  Last week, 3 = the week before that ... up to kMaxWeeksBack weeks ago
// Turn = next / previous page, click = back to the home screen. It never times out.
// All numbers come from app/stats (the session log); this file only draws them.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"

class StatsScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  static constexpr int kMaxWeeksBack = 26;          // about half a year of history to scroll through
  static constexpr int kLastPage = 1 + kMaxWeeksBack;
  static constexpr uint8_t kRows = 4;               // subject rows on the Today page

  void drawPage();
  void drawToday(uint32_t today);
  void drawWeek(uint32_t today, int weeksBack);
  void showTotal(uint32_t seconds, uint32_t sessions);   // the line under the title

  TextField title_, total_, most_, hint_;
  TextField rowName_[kRows], rowTime_[kRows];
  TextField dayLabel_[7];   // weekday initial under each bar
  TextField dayValue_[7];   // the time above each bar
  int page_ = 0;
  bool dirty_ = true;     // the page changed: draw it once in update(), after all queued turns were handled
};
