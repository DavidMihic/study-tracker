#include "app/app_state.h"

namespace AppState {

Session& session() {
  static Session instance;
  return instance;
}

LastBlock& lastBlock() {
  static LastBlock instance;
  return instance;
}

PickerPurpose& pickerPurpose() {
  static PickerPurpose instance = PickerPurpose::ChooseSubject;
  return instance;
}

int& pendingResetSubject() {
  static int instance = -1;
  return instance;
}

ScreenId& settingsReturn() {
  static ScreenId instance = ScreenId::Home;
  return instance;
}

}  // namespace AppState
