// Study tracker firmware entry point. No logic here: set up the drivers, then keep them running.
#include <Arduino.h>
#include "app/app.h"
#include "drivers/backlight.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "drivers/input.h"
#include "drivers/rtc_clock.h"

void setup() {
  Buzzer::begin();       // first: drives the pin HIGH (silent) before anything else can glitch it

  Serial.begin(115200);  // optional: the device runs fine without a PC attached
  Backlight::begin();
  Display::begin();
  Input::begin();
  bool rtcOk = Rtc::begin();
  if (Serial) Serial.println(rtcOk ? F("RTC ok") : F("RTC not found"));

  App::begin();
}

void loop() {
  Input::poll();
  Rtc::poll();
  Buzzer::update();
  App::update();
}
