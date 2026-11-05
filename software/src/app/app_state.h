// State that more than one screen needs. Plain C++ (no Arduino).
#pragma once
#include <stdint.h>
#include "app/navigation.h"
#include "app/session.h"

namespace AppState {

// The running study session / break.
Session& session();

// The block that was just finished, for the break screen to show.
struct LastBlock {
  uint32_t seconds = 0;
  bool counted = false;   // false if it was too short to be recorded
};
LastBlock& lastBlock();

// Where the settings screen returns to (Home or Studying, whichever opened it).
ScreenId& settingsReturn();

}  // namespace AppState
