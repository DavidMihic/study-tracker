#include "app/app.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "app/screens/break.h"
#include "app/screens/home.h"
#include "app/screens/picker.h"
#include "app/screens/screen.h"
#include "app/screens/clock_set.h"
#include "app/screens/reset.h"
#include "app/screens/settings.h"
#include "app/screens/sound_settings.h"
#include "app/screens/stats_screen.h"
#include "app/screens/studying.h"
#include "drivers/input.h"

namespace {

HomeScreen homeScreen;
PickerScreen pickerScreen;
StudyingScreen studyingScreen;
BreakScreen breakScreen;
SettingsScreen settingsScreen;
ClockSetScreen clockSetScreen;
SoundSettingsScreen soundSettingsScreen;
ResetScreen resetScreen;
StatsScreen statsScreen;

Screen* current = nullptr;

}  // namespace

namespace Nav {

void go(ScreenId screen) {
  switch (screen) {
    case ScreenId::Home:     current = &homeScreen; break;
    case ScreenId::Picker:   current = &pickerScreen; break;
    case ScreenId::Studying: current = &studyingScreen; break;
    case ScreenId::Break:    current = &breakScreen; break;
    case ScreenId::Settings: current = &settingsScreen; break;
    case ScreenId::ClockSet: current = &clockSetScreen; break;
    case ScreenId::SoundSettings: current = &soundSettingsScreen; break;
    case ScreenId::Reset: current = &resetScreen; break;
    case ScreenId::Stats: current = &statsScreen; break;
  }
  current->enter();
}

}  // namespace Nav

namespace App {

void begin() {
  // A study block that was running when the power was cut comes back (paused) instead of the home screen.
  bool resume = AppState::session().phase() == Session::Phase::Studying;
  Nav::go(resume ? ScreenId::Studying : ScreenId::Home);
}

void update() {
  Input::Event event;
  // A screen switch inside onEvent() changes `current`, so later events go to the new screen.
  while (Input::next(event)) current->onEvent(event);
  current->update();
}

}  // namespace App
