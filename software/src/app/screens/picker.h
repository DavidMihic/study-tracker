// Subject picker. Shows the subjects of her current semester, favourites first and then the most
// studied. Rows at the bottom open the elective folder and the year/semester chooser.
//   Turn       = move through the list
//   Click      = choose the subject (or open the folder / chooser / go back)
//   Long press = on a subject: add or remove it from the favourites
// Choosing a subject from a semester other than her current one asks "switch to that semester?".
// The focused row with a name that is too long slides its text so the whole name can be read.
#pragma once
#include "app/screens/screen.h"
#include "app/subject_list.h"
#include "data/subjects.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"
#include "ui/scroll_arrows.h"

class PickerScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  // ResetList = every subject that has recorded time (used by Settings > Poništi brojače).
  // Main = subjects of the shown semester, Electives = folder, Years = list of years;
  // Half = full screen with the winter/summer cards, Confirm = full screen "switch semester?".
  enum class Mode : uint8_t { Main, Electives, Years, Half, Confirm, ResetList };

  struct Item {
    enum class Kind : uint8_t { Subject, ElectivesFolder, OtherPeriod, YearChoice, Back };
    Kind kind;
    uint16_t subjectIndex;   // Subject
    uint8_t year;            // YearChoice
  };

  struct RowLook {           // what a row shows besides its text
    bool hasIcon = false;
    Icons::Icon icon = Icons::Icon::Star;
    uint16_t color = 0;
    TextField::IconSide side = TextField::IconSide::None;
  };

  static constexpr uint8_t kVisibleRows = 5;
  static constexpr size_t kMaxItems = Subjects::kMaxSubjects + 4;

  bool cardMode() const { return mode_ == Mode::Half || mode_ == Mode::Confirm; }

  void setupFields();                              // configures the text fields and the static hint
  void redrawScreen();                             // clears the screen and draws the current mode again
  void chooseSubject(size_t subjectIndex);         // remembers it and goes home
  void startConfirm(size_t subjectIndex);          // asks whether to switch semester
  void startHalf(uint8_t year);                    // asks winter or summer for that year
  void rebuild();                                  // fills items_ for the current mode
  int findKind(Item::Kind kind) const;             // position of the first item of that kind, or -1
  int findSubject(size_t subjectIndex) const;      // position of that subject's row, or -1
  void keepSelectionVisible();
  void drawHeader();
  void drawRows();
  void updateMarquee();
  void describe(const Item& item, char* left, size_t leftSize, char* right, size_t rightSize,
                RowLook& look) const;

  Mode mode_ = Mode::Main;
  SubjectList::View view_ = {1, Subjects::kWinter};   // the year and semester being shown
  Item items_[kMaxItems];
  size_t itemCount_ = 0;
  size_t selected_ = 0;   // position in items_
  size_t top_ = 0;        // position of the first visible row

  TextField header_, hint_, question_, subjectName_;
  TextField rowLeft_[kVisibleRows], rowRight_[kVisibleRows];
  CardMenu menu_;
  ScrollArrows arrows_;
  size_t pendingSubject_ = 0;   // Confirm mode: the subject being chosen
  uint8_t pendingYear_ = 1;     // Half mode: the year being chosen
  bool dirty_ = true;

  // Sliding text of the focused row.
  uint32_t listVersion_ = 0;       // changes whenever the list is rebuilt
  size_t marqueeSelected_ = 0;     // the row (and list version) the animation belongs to
  uint32_t marqueeVersion_ = 0;
  uint32_t marqueeStartMs_ = 0;
  int16_t marqueeSkip_ = 0;        // characters currently skipped on the focused row
};
