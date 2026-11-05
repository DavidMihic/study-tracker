// Study tracker firmware entry point. No logic here: set up the drivers, then keep them running.
#include <Arduino.h>
#include "app/app.h"
#include "app/persistence.h"
#include "data/progress.h"
#include "drivers/backlight.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "drivers/flash_store.h"
#include "drivers/input.h"
#include "drivers/rtc_clock.h"

namespace {

uint32_t nowMs() { return millis(); }
uint32_t nowUnix() { return Rtc::isAvailable() ? Rtc::now().unixtime() : 0; }

}  // namespace

void setup() {
  Buzzer::begin();       // first: drives the pin HIGH (silent) before anything else can glitch it

  Serial.begin(115200);  // optional: the device runs fine without a PC attached

  // Load the saved data before the screen turns on, so it starts with the saved brightness.
  Persistence::begin(FlashStore::backend(), {nowMs, nowUnix});
  bool loaded = Persistence::load();
  Backlight::begin();
  Backlight::set(Progress::brightness());
  Display::begin();
  Input::begin();
  bool rtcOk = Rtc::begin();
  if (Serial) {
    Serial.println(rtcOk ? F("RTC ok") : F("RTC not found"));
    Serial.println(loaded ? F("Saved data loaded") : F("No valid saved data, using defaults"));
  }

  Persistence::restoreSession();   // needs the clock
  App::begin();
}

void loop() {
  Input::poll();
  Rtc::poll();
  Buzzer::update();
  App::update();
  Persistence::update();
}
