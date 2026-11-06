// Study tracker firmware entry point. No logic here: set up the drivers, then keep them running.
#include <Arduino.h>
#include "app/app.h"
#include "app/persistence.h"
#include "data/progress.h"
#include "data/session_log.h"
#ifdef DUMMY_DATA
#include "debug/dummy_data.h"
#endif
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
  bool flashOk = FlashStore::begin();
  SessionLog::begin(FlashStore::logBackend());
  Persistence::begin(FlashStore::backend(), {nowMs, nowUnix});
  bool loaded = Persistence::load();
  // The backlight stays off (the pin is low) until the first screen is completely drawn: the ST7789's
  // frame memory holds random data after power-up, which showed as white noise during boot.
  Display::begin();
  Input::begin();
  bool rtcOk = Rtc::begin();
  if (Serial) {
    Serial.println(rtcOk ? F("RTC ok") : F("RTC not found"));
    if (!flashOk) Serial.println(F("Flash file system could not be mounted, nothing will be saved"));
    Serial.println(loaded ? F("Saved data loaded") : F("No valid saved data, using defaults"));
  }

#ifdef DUMMY_DATA                  // only in the feather_demo environment, see debug/dummy_data.h
  DummyData::seedIfEmpty(nowUnix());
#endif

  Persistence::restoreSession();   // needs the clock
  App::begin();

  Backlight::begin();
  Backlight::set(Progress::brightness());
}

void loop() {
  Input::poll();
  Rtc::poll();
  Buzzer::update();
  App::update();
  Persistence::update();
}
