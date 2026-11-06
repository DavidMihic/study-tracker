#include "app/screens/stats_screen.h"
#include "app/navigation.h"
#include "app/stats.h"
#include "data/session_log.h"
#include "data/subjects.h"
#include "drivers/display.h"
#include "drivers/rtc_clock.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/sound.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;

namespace {

// Layout, landscape 320 x 240. The title and the hint stay; the content area between them is redrawn
// when the page changes.
constexpr int16_t kContentTop = 40, kContentHeight = 168;
constexpr int16_t kRowTop = 80, kRowHeight = 28;

// Week chart.
constexpr int16_t kBarsLeft = 13, kBarWidth = 30, kBarGap = 14;
constexpr int16_t kBaseline = 160, kMaxBarHeight = 70;
constexpr int16_t kValueHeight = 16;          // the time label above a bar; the tallest bar leaves room for it
constexpr uint32_t kMinScaleSeconds = 3600;   // a short day is not drawn as a full-height bar

const char* subjectName(uint16_t id) {
  int index = Subjects::indexOfId(id);
  return index >= 0 ? Subjects::at((size_t)index).name : "?";
}

// Per-subject totals; static because kMaxSubjects of them are too much for the stack.
Stats::SubjectTotal subjectRows[Subjects::kMaxSubjects];

}  // namespace

void StatsScreen::enter() {
  Display::clear(Colors::kBackground);

  title_.configure(10, 6, 300, 34, Font::Heading, Align::Center, Colors::kText);
  total_.configure(10, 44, 300, 28, Font::Body, Align::Center, Colors::kGood);
  most_.configure(10, 186, 300, 22, Font::Small, Align::Center, Colors::kMuted);
  hint_.configure(10, 210, 300, 22, Font::Small, Align::Center, Colors::kMuted);
  for (uint8_t i = 0; i < kRows; i++) {
    int16_t y = kRowTop + i * kRowHeight;
    rowName_[i].configure(10, y, 210, kRowHeight, Font::Body, Align::Left, Colors::kText);
    rowTime_[i].configure(220, y, 90, kRowHeight, Font::Body, Align::Right, Colors::kAccent);
  }

  hint_.set(Strings::kStatsHint);
  page_ = 0;
  dirty_ = true;
}

void StatsScreen::onEvent(const Input::Event& event) {
  if (event.type == Input::Event::Type::Click) {
    Sound::press();
    Nav::go(ScreenId::Home);
    return;
  }
  if (event.type != Input::Event::Type::Turn) return;

  int next = page_ + (event.delta > 0 ? 1 : -1);
  if (next < 0 || next > kLastPage) return;     // the ends do not wrap around
  page_ = next;
  dirty_ = true;
  Sound::turn();
}

void StatsScreen::update() {
  if (!dirty_) return;
  dirty_ = false;
  drawPage();
}

void StatsScreen::drawPage() {
  Display::tft().fillRect(0, kContentTop, 320, kContentHeight, Colors::kBackground);
  total_.invalidate();
  most_.invalidate();
  for (uint8_t i = 0; i < kRows; i++) { rowName_[i].invalidate(); rowTime_[i].invalidate(); }

  if (!Rtc::isAvailable()) {                     // without the clock there is no "today"
    title_.set(Strings::kStatsTitle);
    total_.set(Strings::kRtcMissing, Colors::kBad, Colors::kBackground);
    return;
  }
  uint32_t today = SessionLog::studyDay(Rtc::now().unixtime());
  if (page_ == 0) drawToday(today);
  else drawWeek(today, page_ - 1);
}

void StatsScreen::showTotal(uint32_t seconds, uint32_t sessions) {
  if (sessions == 0) {
    total_.set(Strings::kStatsNothing, Colors::kMuted, Colors::kBackground);
    return;
  }
  char duration[24], blocks[24], text[64];
  Format::duration(duration, sizeof duration, seconds);
  Format::blocks(blocks, sizeof blocks, sessions);
  snprintf(text, sizeof text, Strings::kStatsTotalFormat, duration, blocks);
  total_.set(text, Colors::kGood, Colors::kBackground);
}

void StatsScreen::drawToday(uint32_t today) {
  title_.set(Strings::kStatsToday);
  Stats::Totals totals = Stats::total({today, today});
  showTotal(totals.seconds, totals.sessions);
  if (totals.sessions == 0) return;

  size_t n = Stats::perSubject({today, today}, subjectRows, Subjects::kMaxSubjects);
  for (uint8_t i = 0; i < kRows && i < n; i++) {
    char duration[24];
    Format::duration(duration, sizeof duration, subjectRows[i].seconds);
    rowName_[i].set(subjectName(subjectRows[i].subjectId));
    rowTime_[i].set(duration);
  }
}

void StatsScreen::drawWeek(uint32_t today, int weeksBack) {
  const uint32_t first = Stats::weekStart(today) - (uint32_t)weeksBack * 7;
  const uint32_t last = first + 6;

  char title[40];
  if (weeksBack == 0) snprintf(title, sizeof title, "%s", Strings::kStatsThisWeek);
  else if (weeksBack == 1) snprintf(title, sizeof title, "%s", Strings::kStatsLastWeek);
  else Format::dayRange(title, sizeof title, first, last);
  title_.set(title);

  Stats::Totals days[7];
  Stats::perDay(first, 7, days);
  uint32_t seconds = 0, sessions = 0, busiest = 0;
  for (const Stats::Totals& d : days) {
    seconds += d.seconds;
    sessions += d.sessions;
    if (d.seconds > busiest) busiest = d.seconds;
  }
  showTotal(seconds, sessions);
  if (sessions == 0) return;

  // Bars, one per day, Monday first. Today's bar and label are highlighted.
  Adafruit_ST7789& tft = Display::tft();
  const uint32_t scale = busiest > kMinScaleSeconds ? busiest : kMinScaleSeconds;
  for (int i = 0; i < 7; i++) {
    const int16_t x = kBarsLeft + i * (kBarWidth + kBarGap);
    const bool isToday = (first + i == today);
    const uint16_t color = isToday ? Colors::kWarning : Colors::kAccent;

    if (days[i].seconds == 0) {
      tft.fillRect(x, kBaseline - 1, kBarWidth, 2, Colors::kDim);        // an empty day: just a thin line
    } else {
      int16_t height = (int16_t)((uint64_t)days[i].seconds * kMaxBarHeight / scale);
      if (height < 3) height = 3;
      tft.fillRect(x, kBaseline - height, kBarWidth, height, color);

      // The time just above the bar (days without study get no label). The field is as wide as the bar's
      // slot (bar + gap) so neighbouring labels never overlap.
      char value[12];
      Format::barLabel(value, sizeof value, days[i].seconds);
      dayValue_[i].configure(x - kBarGap / 2, kBaseline - height - kValueHeight, kBarWidth + kBarGap, kValueHeight,
                             Font::Small, Align::Center, isToday ? Colors::kWarning : Colors::kText);
      dayValue_[i].set(value);
    }

    dayLabel_[i].configure(x - kBarGap / 2, kBaseline + 6, kBarWidth + kBarGap, 20, Font::Small, Align::Center,
                           isToday ? Colors::kWarning : Colors::kMuted);
    dayLabel_[i].set(Strings::kWeekdayInitials[i]);
  }

  // The subject she studied most this week.
  size_t n = Stats::perSubject({first, last}, subjectRows, Subjects::kMaxSubjects);
  if (n > 0) {
    char text[96];
    snprintf(text, sizeof text, Strings::kStatsMostStudied, subjectName(subjectRows[0].subjectId));
    most_.set(text);
  }
}
