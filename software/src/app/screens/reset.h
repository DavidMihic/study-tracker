// Reset counters (settings submenu): set the time and session count of ONE subject or of ALL subjects
// back to 0. Favourites, the last subject and the settings stay. Nothing is reset by a single press:
// a confirmation screen with two cards (Natrag / Poništi) comes first, it starts on Natrag, and it never
// closes by itself.
//   Menu:    Jedan predmet (opens the subject picker in "reset" mode) / Svi predmeti / Natrag
//   Confirm: shows what will be cleared (time and number of sessions)
#pragma once
#include "app/screens/screen.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"

class ResetScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum class Mode : uint8_t { Menu, Confirm };
  enum Row : uint8_t { kOne, kAll, kBack, kRowCount };

  void showMenu();
  void showConfirm(int subjectIndex);    // subjectIndex < 0 = all subjects
  void drawRows();
  void applyReset();

  Mode mode_ = Mode::Menu;
  int target_ = -1;          // Confirm: the subject to reset, or -1 for all
  TextField title_, hint_, question_, name_, details_;
  TextField label_[kRowCount];
  CardMenu menu_;
  uint8_t selected_ = 0;
  bool dirty_ = true;
};
