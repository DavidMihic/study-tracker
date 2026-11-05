#include "app/screens/picker.h"
#include "app/navigation.h"
#include "app/semester.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/strings.h"

using Align = TextField::Align;
using Display::Font;
using Icons::Icon;

namespace {

// Layout, landscape 320 x 240.
constexpr int16_t kHeaderY = 4;
constexpr int16_t kHeaderHeight = 26;
constexpr int16_t kRowsY = 34;
constexpr int16_t kRowHeight = 32;
constexpr int16_t kLeftWidth = 214;    // subject name
constexpr int16_t kRightWidth = 106;   // total study time
constexpr int16_t kHintY = 204;

}  // namespace

void PickerScreen::setupFields() {
  header_.configure(0, kHeaderY, 320, kHeaderHeight, Font::Body, Align::Left, Colors::kAccent,
                    Colors::kBackground, 6);
  hint_.configure(0, kHintY, 320, 22, Font::Small, Align::Center, Colors::kMuted);
  question_.configure(0, 40, 320, 34, Font::Heading, Align::Center, Colors::kText);
  subjectName_.configure(0, 84, 320, 30, Font::Body, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kVisibleRows; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    rowLeft_[r].configure(0, y, kLeftWidth, kRowHeight, Font::Body, Align::Left, Colors::kText,
                          Colors::kBackground, 6);
    rowRight_[r].configure(kLeftWidth, y, kRightWidth, kRowHeight, Font::Small, Align::Right,
                           Colors::kMuted, Colors::kBackground, 6);
  }
  hint_.set(Strings::kPickerHint);
}

void PickerScreen::enter() {
  Display::clear(Colors::kBackground);
  menu_.close();
  setupFields();

  // Start on her current semester, with the subject she studies now selected.
  uint8_t semester = Progress::currentSemester();
  view_ = {Semesters::yearOf(semester), Semesters::halfOf(semester)};
  mode_ = Mode::Main;
  rebuild();

  int current = Progress::lastSubjectIndex();
  int position = (current >= 0) ? findSubject((size_t)current) : -1;
  selected_ = (position >= 0) ? (size_t)position : 0;
  top_ = 0;
  keepSelectionVisible();
  dirty_ = true;
}

void PickerScreen::chooseSubject(size_t subjectIndex) {
  Progress::setLastSubject(subjectIndex);
  Buzzer::play(Buzzer::Pattern::Short);
  Nav::go(ScreenId::Home);
}

void PickerScreen::startConfirm(size_t subjectIndex) {
  pendingSubject_ = subjectIndex;
  mode_ = Mode::Confirm;
  Display::clear(Colors::kBackground);
  setupFields();

  char text[48];
  snprintf(text, sizeof text, Strings::kSwitchQuestion, (unsigned)Semesters::number(view_.year, view_.semester));
  question_.set(text);
  subjectName_.set(Subjects::at(subjectIndex).name);
  hint_.set("");

  const CardMenu::Card cards[] = {
    {Icon::Check, Strings::kCardYes},
    {Icon::Cross, Strings::kCardNo},
  };
  menu_.open(cards, 2, millis());
}

void PickerScreen::leaveConfirm() {
  menu_.close();
  mode_ = Mode::Main;
  Display::clear(Colors::kBackground);
  setupFields();
  dirty_ = true;
}

void PickerScreen::rebuild() {
  itemCount_ = 0;
  size_t list[Subjects::kMaxSubjects];

  auto add = [this](Item::Kind kind, size_t subjectIndex = 0, uint8_t year = 0, uint8_t semester = 0) {
    items_[itemCount_++] = {kind, (uint16_t)subjectIndex, year, semester};
  };

  switch (mode_) {
    case Mode::Main: {
      size_t n = SubjectList::buildMain(view_, list, Subjects::kMaxSubjects);
      for (size_t i = 0; i < n; i++) add(Item::Kind::Subject, list[i]);
      add(Item::Kind::ElectivesFolder);
      add(Item::Kind::OtherPeriod);
      add(Item::Kind::Back);
      break;
    }
    case Mode::Electives: {
      size_t n = SubjectList::buildElectives(view_, list, Subjects::kMaxSubjects);
      for (size_t i = 0; i < n; i++) add(Item::Kind::Subject, list[i]);
      add(Item::Kind::Back);
      break;
    }
    case Mode::Confirm:
      break;
    case Mode::Period: {
      for (uint8_t year = StudyRules::kFirstStudyYear; year <= StudyRules::kLastStudyYear; year++) {
        add(Item::Kind::PeriodChoice, 0, year, Subjects::kWinter);
        add(Item::Kind::PeriodChoice, 0, year, Subjects::kSummer);
      }
      add(Item::Kind::Back);
      break;
    }
  }
  if (selected_ >= itemCount_) selected_ = itemCount_ - 1;
}

int PickerScreen::findKind(Item::Kind kind) const {
  for (size_t i = 0; i < itemCount_; i++) {
    if (items_[i].kind == kind) return (int)i;
  }
  return -1;
}

int PickerScreen::findSubject(size_t subjectIndex) const {
  for (size_t i = 0; i < itemCount_; i++) {
    if (items_[i].kind == Item::Kind::Subject && items_[i].subjectIndex == subjectIndex) return (int)i;
  }
  return -1;
}

void PickerScreen::keepSelectionVisible() {
  if (selected_ < top_) top_ = selected_;
  if (selected_ >= top_ + kVisibleRows) top_ = selected_ - kVisibleRows + 1;
}

void PickerScreen::onEvent(const Input::Event& event) {
  if (mode_ == Mode::Confirm) {
    if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
    if (menu_.chosen() == 0) Progress::setCurrentSemester(Semesters::number(view_.year, view_.semester));
    chooseSubject(pendingSubject_);   // "Da" switches the semester too; "Ne" only picks the subject
    return;
  }

  switch (event.type) {
    case Input::Event::Type::Turn: {
      int target = (int)selected_ + event.delta;
      if (target < 0 || target >= (int)itemCount_) break;   // stops at the ends
      selected_ = (size_t)target;
      keepSelectionVisible();
      Buzzer::play(Buzzer::Pattern::Click);
      dirty_ = true;
      break;
    }

    case Input::Event::Type::Click: {
      const Item item = items_[selected_];
      switch (item.kind) {
        case Item::Kind::Subject:
          if (Semesters::number(view_.year, view_.semester) != Progress::currentSemester()) {
            Buzzer::play(Buzzer::Pattern::Short);
            startConfirm(item.subjectIndex);
          } else {
            chooseSubject(item.subjectIndex);
          }
          return;

        case Item::Kind::ElectivesFolder:
          mode_ = Mode::Electives;
          rebuild();
          selected_ = 0;
          top_ = 0;
          break;

        case Item::Kind::OtherPeriod: {
          mode_ = Mode::Period;
          rebuild();
          selected_ = 0;
          for (size_t i = 0; i < itemCount_; i++) {   // start on the period being shown now
            if (items_[i].kind == Item::Kind::PeriodChoice && items_[i].year == view_.year &&
                items_[i].semester == view_.semester) {
              selected_ = i;
            }
          }
          top_ = 0;
          keepSelectionVisible();
          break;
        }

        case Item::Kind::PeriodChoice:
          view_ = {item.year, item.semester};
          mode_ = Mode::Main;
          rebuild();
          selected_ = 0;
          top_ = 0;
          break;

        case Item::Kind::Back: {
          if (mode_ == Mode::Main) {
            Buzzer::play(Buzzer::Pattern::Short);
            Nav::go(ScreenId::Home);
            return;
          }
          Item::Kind cameFrom = (mode_ == Mode::Electives) ? Item::Kind::ElectivesFolder : Item::Kind::OtherPeriod;
          mode_ = Mode::Main;
          rebuild();
          int position = findKind(cameFrom);
          selected_ = (position >= 0) ? (size_t)position : 0;
          top_ = 0;
          keepSelectionVisible();
          break;
        }
      }
      Buzzer::play(Buzzer::Pattern::Short);
      dirty_ = true;
      break;
    }

    case Input::Event::Type::LongPress: {
      const Item item = items_[selected_];
      if (item.kind != Item::Kind::Subject) break;
      Progress::toggleFavorite(item.subjectIndex);
      rebuild();                                       // favourites move to the top: follow the subject
      int position = findSubject(item.subjectIndex);
      if (position >= 0) selected_ = (size_t)position;
      keepSelectionVisible();
      Buzzer::play(Buzzer::Pattern::Short);
      dirty_ = true;
      break;
    }
  }
}

void PickerScreen::update() {
  if (mode_ == Mode::Confirm) {
    if (menu_.update(millis()) == CardMenu::Result::TimedOut) leaveConfirm();
    return;
  }
  if (!dirty_) return;
  drawHeader();
  drawRows();
  dirty_ = false;
}

void PickerScreen::drawHeader() {
  char text[48];
  switch (mode_) {
    case Mode::Main:      Format::period(text, sizeof text, view_.year, view_.semester); break;
    case Mode::Electives: snprintf(text, sizeof text, "%s", Strings::kElectivesFolder); break;
    case Mode::Period:    snprintf(text, sizeof text, "%s", Strings::kPeriodTitle); break;
    case Mode::Confirm:   return;
  }
  header_.set(text);
}

void PickerScreen::describe(const Item& item, char* left, size_t leftSize, char* right, size_t rightSize) const {
  right[0] = '\0';
  switch (item.kind) {
    case Item::Kind::Subject: {
      const SubjectStats& stats = Progress::stats(item.subjectIndex);
      snprintf(left, leftSize, "%s%s", stats.favorite ? Strings::kFavoriteMarker : "",
               Subjects::at(item.subjectIndex).name);
      if (stats.totalSeconds > 0) Format::duration(right, rightSize, stats.totalSeconds);
      break;
    }
    case Item::Kind::ElectivesFolder:
      snprintf(left, leftSize, "%s%s", Strings::kElectivesFolder, Strings::kFolderMarker);
      break;
    case Item::Kind::OtherPeriod:
      snprintf(left, leftSize, "%s%s", Strings::kOtherPeriod, Strings::kFolderMarker);
      break;
    case Item::Kind::PeriodChoice:
      Format::period(left, leftSize, item.year, item.semester);
      break;
    case Item::Kind::Back:
      snprintf(left, leftSize, "< %s", Strings::kBack);
      break;
  }
}

void PickerScreen::drawRows() {
  for (uint8_t r = 0; r < kVisibleRows; r++) {
    size_t position = top_ + r;
    if (position >= itemCount_) {
      rowLeft_[r].set("", Colors::kText, Colors::kBackground);
      rowRight_[r].set("", Colors::kMuted, Colors::kBackground);
      continue;
    }
    char left[64], right[24];
    describe(items_[position], left, sizeof left, right, sizeof right);

    bool selected = (position == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;
    rowLeft_[r].set(left, Colors::kText, background);
    rowRight_[r].set(right, selected ? Colors::kText : Colors::kMuted, background);
  }
}
