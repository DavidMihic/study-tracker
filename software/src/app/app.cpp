#include "app/app.h"
#include "app/navigation.h"
#include "app/screens/break.h"
#include "app/screens/home.h"
#include "app/screens/picker.h"
#include "app/screens/screen.h"
#include "app/screens/clock_set.h"
#include "app/screens/settings.h"
#include "app/screens/studying.h"
#include "drivers/input.h"

namespace {

HomeScreen homeScreen;
PickerScreen pickerScreen;
StudyingScreen studyingScreen;
BreakScreen breakScreen;
SettingsScreen settingsScreen;
ClockSetScreen clockSetScreen;

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
  }
  current->enter();
}

}  // namespace Nav

namespace App {

void begin() { Nav::go(ScreenId::Home); }

void update() {
  Input::Event event;
  // A screen switch inside onEvent() changes `current`, so later events go to the new screen.
  while (Input::next(event)) current->onEvent(event);
  current->update();
}

}  // namespace App
