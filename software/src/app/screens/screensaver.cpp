#include "app/screens/screensaver.h"
#include "app/navigation.h"
#include "config/config.h"
#include "data/progress.h"
#include "drivers/backlight.h"
#include "drivers/display.h"
#include "drivers/rtc_clock.h"
#include "ui/colors.h"
#include "ui/ecg.h"
#include "ui/format.h"

using Align = TextField::Align;
using Display::Font;

namespace {

constexpr int16_t kWidth = Config::kScreenWidth;
constexpr int16_t kBaseline = 190;                    // y of the flat part of the trace
constexpr int16_t kBandTop = kBaseline - Ecg::kMaxUp - 4;   // 3 px thick line: one pixel of margin is enough
constexpr int16_t kBandHeight = Ecg::kMaxUp + Ecg::kMaxDown + 8;
constexpr uint32_t kPixelsPerSecond = Ecg::kBeatWidth;   // one beat per second (60 bpm)
constexpr int16_t kGapAhead = 10;                     // the erased gap in front of the moving line
constexpr uint16_t kTraceColor = Colors::kGood;

int16_t yAt(uint32_t pixel) { return kBaseline + Ecg::offset((int)(pixel % Ecg::kBeatWidth)); }

}  // namespace

void ScreensaverScreen::enter() {
  Display::clear(Colors::kBackground);
  Backlight::set(min<int>(Progress::brightness(), Config::kScreensaverBrightness));

  clock_.configure(10, 24, 300, 54, Font::Clock, Align::Center, Colors::kAccent);
  date_.configure(10, 84, 300, 26, Font::Body, Align::Center, Colors::kMuted);

  startMs_ = millis();
  lastPixel_ = 0;
  lastY_ = yAt(0);
  redrawClock_ = true;
}

void ScreensaverScreen::onEvent(const Input::Event&) {
  Nav::go(ScreenId::Home);                           // the event is used up by waking the device
  Backlight::set(Progress::brightness());            // after Home has drawn, so no bright flash
}

void ScreensaverScreen::update() {
  if (Rtc::secondChanged() || redrawClock_) {
    drawClock();
    redrawClock_ = false;
  }
  drawTrace();
}

void ScreensaverScreen::drawClock() {
  char text[40];
  if (Rtc::isAvailable()) {
    DateTime now = Rtc::now();
    Format::clock(text, sizeof text, now.hour(), now.minute(), now.second());
    clock_.set(text);
    Format::date(text, sizeof text, now.dayOfTheWeek(), now.day(), now.month(), now.year());
    date_.set(text);
  } else {
    clock_.set("--:--:--");
  }
}

void ScreensaverScreen::drawTrace() {
  auto& tft = Display::tft();
  const uint32_t pixel = (uint32_t)((uint64_t)(millis() - startMs_) * kPixelsPerSecond / 1000);
  if (pixel == lastPixel_) return;

  uint32_t from = lastPixel_ + 1;
  if (pixel - lastPixel_ > (uint32_t)kWidth) from = pixel - kWidth + 1;   // stalled for a long time: skip ahead

  for (uint32_t p = from; p <= pixel; p++) {
    const int16_t x = p % kWidth;
    const int16_t y = yAt(p);
    tft.fillRect((x + kGapAhead) % kWidth, kBandTop, 1, kBandHeight, Colors::kBackground);   // erase ahead
    if (x == 0 || p != lastPixel_ + 1) {
      tft.fillRect(x, y - 1, 2, 3, kTraceColor);   // start of a sweep: a 2 x 3 dot
    } else {
      // Four copies of the line, shifted one pixel up, down and right, make it 3 px thick (and 2 px
      // wide on the steep parts, where shifting up/down alone would leave it thin).
      tft.drawLine(x - 1, lastY_ - 1, x, y - 1, kTraceColor);
      tft.drawLine(x - 1, lastY_,     x, y,     kTraceColor);
      tft.drawLine(x - 1, lastY_ + 1, x, y + 1, kTraceColor);
      tft.drawLine(x,     lastY_,     x + 1, y, kTraceColor);
    }
    lastY_ = y;
    lastPixel_ = p;
  }
  lastPixel_ = pixel;
}
