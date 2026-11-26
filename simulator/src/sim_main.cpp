// SIMULATOR ONLY. The counterpart of software/src/main.cpp (same set-up order and loop), plus the
// functions the web page calls. If main.cpp changes, mirror the change here.
#include <Arduino.h>
#include <emscripten.h>
#include "app/app.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "data/progress.h"
#include "data/session_log.h"
#include "data/subjects.h"
#include "drivers/backlight.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "drivers/flash_store.h"
#include "drivers/input.h"
#include "drivers/rtc_clock.h"
#include "sim.h"
#if __has_include("debug/dummy_data.h")
#define SIM_HAS_DEMO_DATA 1
#include "debug/dummy_data.h"
#endif

namespace {

uint32_t nowMs() { return millis(); }
uint32_t nowUnix() { return Rtc::isAvailable() ? Rtc::now().unixtime() : 0; }

// Made-up study history for the first start. The firmware's demo data only fills the session log (the
// statistics screen), so the totals shown in the subject list are added up from it here, and the two
// most recently studied subjects are marked as favourite / last studied.
void seedDemoData() {
#ifdef SIM_HAS_DEMO_DATA
  DummyData::seedIfEmpty(nowUnix());
  static int lastIndex;
  lastIndex = -1;
  SessionLog::forEach([](const SessionLog::Record& record, void*) {
    const int index = Subjects::indexOfId(record.subjectId);
    if (index < 0) return;
    Progress::recordSession((size_t)index, record.seconds);
    lastIndex = index;
  }, nullptr);
  if (lastIndex >= 0) {
    Progress::setFavorite((size_t)lastIndex, true);
    Progress::setLastSubject((size_t)lastIndex);
  }
  Persistence::saveNow();
#endif
}

}  // namespace

extern "C" {

// hostMs: the page's millisecond counter. clockUnix: local wall-clock time. seed: first start.
EMSCRIPTEN_KEEPALIVE void sim_setup(double hostMs, double clockUnix, int seed) {
  Sim::setHostMs(hostMs);
  Sim::setClock((uint32_t)clockUnix);

  Buzzer::begin();
  FlashStore::begin();
  SessionLog::begin(FlashStore::logBackend());
  Persistence::begin(FlashStore::backend(), {nowMs, nowUnix});
  const bool loaded = Persistence::load();
  Display::begin();
  Input::begin();
  Rtc::begin();

  if (seed && !loaded) seedDemoData();

  Persistence::restoreSession();
  App::begin();

  Backlight::begin();
  Backlight::set(Progress::brightness());
}

// One pass of loop().
EMSCRIPTEN_KEEPALIVE void sim_loop(double hostMs) {
  Sim::setHostMs(hostMs);
  Input::poll();
  Rtc::poll();
  Buzzer::update();
  App::update();
  Persistence::update();
}

EMSCRIPTEN_KEEPALIVE void sim_turn(int clicks) { Sim::turn(clicks); }
EMSCRIPTEN_KEEPALIVE void sim_button(int down) { Sim::setButton(down != 0); }
EMSCRIPTEN_KEEPALIVE void sim_skip(int seconds) { Sim::skipMs((uint32_t)seconds * 1000u); }

// Demo shortcut instead of waiting two minutes. Waking the screensaver goes to the home screen, so it is
// refused while a study block or break is running. Returns 1 if it started.
EMSCRIPTEN_KEEPALIVE int sim_screensaver() {
  if (AppState::session().phase() != Session::Phase::Idle) return 0;
  Nav::go(ScreenId::Screensaver);
  return 1;
}

EMSCRIPTEN_KEEPALIVE const uint16_t* sim_frame() { return Display::tft().frame; }

// True if something was drawn since the last call.
EMSCRIPTEN_KEEPALIVE int sim_take_dirty() {
  const bool dirty = Display::tft().dirty;
  Display::tft().dirty = false;
  return dirty;
}

EMSCRIPTEN_KEEPALIVE int sim_backlight() { return Sim::backlight(); }
EMSCRIPTEN_KEEPALIVE double sim_clock() { return Sim::clockNow(); }

}  // extern "C"
