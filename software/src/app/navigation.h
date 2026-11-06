// Switching between screens. Screens call Nav::go(); app.cpp does the actual switch.
#pragma once

enum class ScreenId : unsigned char { Home, Picker, Studying, Break, Settings, ClockSet, SoundSettings, Reset, Stats };

namespace Nav {

void go(ScreenId screen);

}  // namespace Nav
