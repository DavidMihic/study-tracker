// Break after a study block: a count-up timer. A click opens the card menu: a new block on the same
// subject, or back to the home screen. Nothing forces her to do either.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"

class BreakScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  void closeMenu();

  TextField title_, timer_, lastBlock_, hint_;
  CardMenu menu_;
  bool redraw_ = true;
};
