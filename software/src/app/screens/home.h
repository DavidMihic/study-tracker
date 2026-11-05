// Home screen: the subject she will study (the one from last time), the clock and the date.
// Click opens the card menu: start studying, choose a subject, settings.
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"

class HomeScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  void drawTime();
  void drawSubject();
  void drawBottom();      // the notice and the hint, which the card menu covers while it is open
  void openMenu();
  void closeMenu();

  // Position in the Subjects list of the subject to study, or -1 if there is none.
  int selectedSubject() const;

  TextField subject_, clock_, date_, notice_, hint_;
  CardMenu menu_;
  bool redrawTime_ = true;
};
