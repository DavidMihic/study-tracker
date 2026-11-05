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

// What the subject picker is opened for. It reads (and resets) this in enter().
enum class PickerPurpose : uint8_t { ChooseSubject, ResetOne };
PickerPurpose& pickerPurpose();

// The reset-counters flow: the picker leaves the chosen subject (position in the list) here for the
// reset screen, which then asks for confirmation. -1 = none.
int& pendingResetSubject();

// Where the settings screen returns to (Home or Studying, whichever opened it).
ScreenId& settingsReturn();

}  // namespace AppState
