// Studying: a count-up timer. At the target time there is one gentle nudge (soft beeps and a colour
// change), then it just keeps counting. Nothing ever stops it except her. A click opens the card menu:
// pause / resume, stop, settings (the menu closes by itself after 15 s and then changes nothing).
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"

class StudyingScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum class Shown : uint8_t { Running, Paused, Reached };

  void openMenu();
  void closeMenu();
  void stopStudying();
  void drawStatus(Shown state);   // the line under the timer and the timer colour
  void drawTimer();

  TextField subject_, timer_, target_, hint_;
  CardMenu menu_;
  bool redraw_ = true;
  Shown shown_ = Shown::Running;
  bool statusDrawn_ = false;

  // Power came back while the block was running: beep until she touches the device, so the block can
  // never sit paused unnoticed.
  bool alertActive_ = false;
  uint32_t nextAlertMs_ = 0;
};
