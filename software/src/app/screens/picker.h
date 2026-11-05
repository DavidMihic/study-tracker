// Subject picker. Shows the subjects of the current year and semester, favourites first and then the
// most studied. Rows at the bottom open the elective folder and the year/semester chooser.
//   Turn       = move through the list
//   Click      = choose the subject (or open the folder / chooser / go back)
//   Long press = on a subject: add or remove it from the favourites
// Choosing a subject from a semester other than her current one asks "switch to that semester?".
#pragma once
#include "app/screens/screen.h"
#include "app/subject_list.h"
#include "data/subjects.h"
#include "drivers/text_field.h"
#include "ui/card_menu.h"

class PickerScreen : public Screen {
 public:
  void enter() override;
  void onEvent(const Input::Event& event) override;
  void update() override;

 private:
  enum class Mode : uint8_t { Main, Electives, Period, Confirm };

  struct Item {
    enum class Kind : uint8_t { Subject, ElectivesFolder, OtherPeriod, PeriodChoice, Back };
    Kind kind;
    uint16_t subjectIndex;   // Subject
    uint8_t year;            // PeriodChoice
    uint8_t semester;        // PeriodChoice
  };

  static constexpr uint8_t kVisibleRows = 5;
  static constexpr size_t kMaxItems = Subjects::kMaxSubjects + 4;

  void setupFields();                              // configures the text fields and the static hint
  void chooseSubject(size_t subjectIndex);         // remembers it and goes home
  void startConfirm(size_t subjectIndex);          // asks whether to switch semester
  void leaveConfirm();                             // back to the list, nothing chosen
  void rebuild();                                  // fills items_ for the current mode
  int findKind(Item::Kind kind) const;             // position of the first item of that kind, or -1
  int findSubject(size_t subjectIndex) const;      // position of that subject's row, or -1
  void keepSelectionVisible();
  void drawHeader();
  void drawRows();
  void describe(const Item& item, char* left, size_t leftSize, char* right, size_t rightSize) const;

  Mode mode_ = Mode::Main;
  SubjectList::View view_ = {1, Subjects::kWinter};   // the year and semester being shown
  Item items_[kMaxItems];
  size_t itemCount_ = 0;
  size_t selected_ = 0;   // position in items_
  size_t top_ = 0;        // position of the first visible row

  TextField header_, hint_, question_, subjectName_;
  CardMenu menu_;
  size_t pendingSubject_ = 0;   // Confirm mode: the subject being chosen
  TextField rowLeft_[kVisibleRows], rowRight_[kVisibleRows];
  bool dirty_ = true;
};
