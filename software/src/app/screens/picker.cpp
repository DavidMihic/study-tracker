#include "app/screens/picker.h"
#include "app/app_state.h"
#include "app/navigation.h"
#include "app/persistence.h"
#include "app/semester.h"
#include "config/study_rules.h"
#include "data/progress.h"
#include "drivers/display.h"
#include "ui/colors.h"
#include "ui/format.h"
#include "ui/sound.h"
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
constexpr int16_t kScreenWidth = 320;
constexpr int16_t kTextWidth = kScreenWidth - ScrollArrows::kReservedWidth;   // title and hint leave the corners free
constexpr int16_t kRightPadding = 6;   // each side of the study-time text
constexpr int16_t kHintY = 204;

// Sliding name of the focused row: wait, then one character per step, hold at the end, repeat.
constexpr uint32_t kMarqueeWaitMs = 900;
constexpr uint32_t kMarqueeStepMs = 140;
constexpr uint32_t kMarqueeHoldMs = 1200;

}  // namespace

void PickerScreen::setupFields() {
  header_.configure(0, kHeaderY, kTextWidth, kHeaderHeight, Font::Body, Align::Left, Colors::kAccent,
                    Colors::kBackground, 6);
  hint_.configure(0, kHintY, kTextWidth, 22, Font::Small, Align::Center, Colors::kMuted);
  question_.configure(0, 40, 320, 34, Font::Heading, Align::Center, Colors::kText);
  subjectName_.configure(0, 84, 320, 30, Font::Body, Align::Center, Colors::kMuted);
  for (uint8_t r = 0; r < kVisibleRows; r++) {
    int16_t y = kRowsY + r * kRowHeight;
    // The two fields share the row; drawRows() gives the study time only the width it needs.
    rowLeft_[r].configure(0, y, kScreenWidth, kRowHeight, Font::Body, Align::Left, Colors::kText,
                          Colors::kBackground, 6);
    rowRight_[r].configure(kScreenWidth, y, 0, kRowHeight, Font::Small, Align::Right,
                           Colors::kMuted, Colors::kBackground, kRightPadding);
  }
  arrows_.reset();
  if (cardMode()) hint_.set("");   // the other modes set their hint in drawHeader()
}

void PickerScreen::redrawScreen() {
  menu_.close();
  Display::clear(Colors::kBackground);
  setupFields();
  dirty_ = true;
}

void PickerScreen::enter() {
  // The reset flow opens the picker with a different purpose (read once).
  bool resetPick = AppState::pickerPurpose() == AppState::PickerPurpose::ResetOne;
  AppState::pickerPurpose() = AppState::PickerPurpose::ChooseSubject;

  mode_ = resetPick ? Mode::ResetList : Mode::Main;
  redrawScreen();
  if (resetPick) {
    rebuild();
    selected_ = 0;
    top_ = 0;
    return;
  }

  // Start on her current semester, with the subject she studies now selected.
  uint8_t semester = Progress::currentSemester();
  view_ = {Semesters::yearOf(semester), Semesters::halfOf(semester)};
  rebuild();

  int current = Progress::lastSubjectIndex();
  int position = (current >= 0) ? findSubject((size_t)current) : -1;
  selected_ = (position >= 0) ? (size_t)position : 0;
  top_ = 0;
  keepSelectionVisible();
}

void PickerScreen::chooseSubject(size_t subjectIndex) {
  Progress::setLastSubject(subjectIndex);
  Persistence::requestSave();   // also covers a semester switch made just before
  Sound::press();
  Nav::go(ScreenId::Home);
}

void PickerScreen::startConfirm(size_t subjectIndex) {
  pendingSubject_ = subjectIndex;
  mode_ = Mode::Confirm;
  redrawScreen();

  char text[48];
  snprintf(text, sizeof text, Strings::kSwitchQuestion, (unsigned)Semesters::number(view_.year, view_.semester));
  question_.set(text);
  subjectName_.set(Subjects::at(subjectIndex).name);

  const CardMenu::Card cards[] = {
    {Icon::Check, Strings::kCardYes},
    {Icon::Cross, Strings::kCardNo},
  };
  menu_.open(cards, 2, millis());
}

void PickerScreen::startHalf(uint8_t year) {
  pendingYear_ = year;
  mode_ = Mode::Half;
  redrawScreen();

  char text[48];
  snprintf(text, sizeof text, Strings::kYearFormat, (unsigned)year);
  question_.set(text);
  subjectName_.set(Strings::kHalfPrompt);

  const CardMenu::Card cards[] = {
    {Icon::Back, Strings::kBack},
    {Icon::Snowflake, Strings::kHalfWinter},
    {Icon::Sun, Strings::kHalfSummer},
  };
  // Start on the half she is looking at now, if it is the same year. No timeout: it waits for her.
  uint8_t start = (year == view_.year && view_.semester == Subjects::kSummer) ? 2 : 1;
  menu_.open(cards, 3, millis(), start, 0);
}

void PickerScreen::rebuild() {
  listVersion_++;
  itemCount_ = 0;
  size_t list[Subjects::kMaxSubjects];

  auto add = [this](Item::Kind kind, size_t subjectIndex = 0, uint8_t year = 0) {
    items_[itemCount_++] = {kind, (uint16_t)subjectIndex, year};
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
    case Mode::ResetList: {
      size_t n = SubjectList::buildStudied(list, Subjects::kMaxSubjects);
      for (size_t i = 0; i < n; i++) add(Item::Kind::Subject, list[i]);
      add(Item::Kind::Back);
      break;
    }
    case Mode::Years: {
      add(Item::Kind::Back);   // also first, so a wrong turn into this list needs no long scroll
      for (uint8_t year = StudyRules::kFirstStudyYear; year <= StudyRules::kLastStudyYear; year++) {
        add(Item::Kind::YearChoice, 0, year);
      }
      add(Item::Kind::Back);
      break;
    }
    case Mode::Half:
    case Mode::Confirm:
      break;   // full-screen cards, no list
  }
  if (itemCount_ == 0) return;
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
  if (cardMode()) {
    if (menu_.onEvent(event, millis()) != CardMenu::Result::Chosen) return;
    uint8_t choice = menu_.chosen();

    if (mode_ == Mode::Confirm) {
      if (choice == 0) Progress::setCurrentSemester(Semesters::number(view_.year, view_.semester));
      chooseSubject(pendingSubject_);   // "Da" switches the semester too; "Ne" only picks the subject
      return;
    }

    Sound::press();
    if (choice == 0) {   // Half: Back = to the list of years
      mode_ = Mode::Years;
      redrawScreen();
      rebuild();
      for (size_t i = 0; i < itemCount_; i++) {
        if (items_[i].kind == Item::Kind::YearChoice && items_[i].year == pendingYear_) selected_ = i;
      }
      top_ = 0;
      keepSelectionVisible();
      return;
    }
    // Half: winter or summer of the chosen year, then back to the subject list of that semester.
    view_ = {pendingYear_, choice == 1 ? Subjects::kWinter : Subjects::kSummer};
    mode_ = Mode::Main;
    redrawScreen();
    rebuild();
    selected_ = 0;
    top_ = 0;
    return;
  }

  switch (event.type) {
    case Input::Event::Type::Turn: {
      int target = (int)selected_ + event.delta;
      if (target < 0 || target >= (int)itemCount_) break;   // stops at the ends
      selected_ = (size_t)target;
      keepSelectionVisible();
      Sound::turn();
      dirty_ = true;
      break;
    }

    case Input::Event::Type::Click: {
      const Item item = items_[selected_];
      switch (item.kind) {
        case Item::Kind::Subject:
          if (mode_ == Mode::ResetList) {          // go on to the confirmation screen
            Sound::press();
            AppState::pendingResetSubject() = (int)item.subjectIndex;
            Nav::go(ScreenId::Reset);
            return;
          }
          if (Semesters::number(view_.year, view_.semester) != Progress::currentSemester()) {
            Sound::press();
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
          mode_ = Mode::Years;
          rebuild();
          selected_ = 0;
          for (size_t i = 0; i < itemCount_; i++) {   // start on the year being shown now
            if (items_[i].kind == Item::Kind::YearChoice && items_[i].year == view_.year) selected_ = i;
          }
          top_ = 0;
          keepSelectionVisible();
          break;
        }

        case Item::Kind::YearChoice:
          Sound::press();
          startHalf(item.year);
          return;

        case Item::Kind::Back: {
          if (mode_ == Mode::ResetList) {
            Sound::press();
            Nav::go(ScreenId::Reset);
            return;
          }
          if (mode_ == Mode::Main) {
            Sound::press();
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
      Sound::press();
      dirty_ = true;
      break;
    }

    case Input::Event::Type::LongPress: {
      const Item item = items_[selected_];
      if (item.kind != Item::Kind::Subject || mode_ == Mode::ResetList) break;
      Progress::toggleFavorite(item.subjectIndex);
      Persistence::requestSave();
      rebuild();                                       // favourites move to the top: follow the subject
      int position = findSubject(item.subjectIndex);
      if (position >= 0) selected_ = (size_t)position;
      keepSelectionVisible();
      Sound::press();
      dirty_ = true;
      break;
    }
  }
}

void PickerScreen::update() {
  if (cardMode()) {
    // Only the "switch semester?" question times out (back to the list, nothing chosen). The
    // winter/summer screen has its own Back card and waits for her.
    if (menu_.update(millis()) == CardMenu::Result::TimedOut) {
      mode_ = Mode::Main;
      redrawScreen();
    }
    return;
  }

  if (dirty_) {
    drawHeader();
    drawRows();
    dirty_ = false;
    return;
  }
  updateMarquee();
}

void PickerScreen::updateMarquee() {
  if (selected_ < top_ || selected_ >= top_ + kVisibleRows) return;
  TextField& field = rowLeft_[selected_ - top_];
  if (!field.overflows()) return;

  int16_t steps = field.scrollSteps();
  uint32_t cycle = kMarqueeWaitMs + (uint32_t)steps * kMarqueeStepMs + kMarqueeHoldMs;
  uint32_t t = (millis() - marqueeStartMs_) % cycle;
  int16_t skip = 0;
  if (t >= kMarqueeWaitMs) {
    uint32_t stepsDone = (t - kMarqueeWaitMs) / kMarqueeStepMs;
    skip = (stepsDone > (uint32_t)steps) ? steps : (int16_t)stepsDone;
  }
  if (skip != marqueeSkip_) {
    marqueeSkip_ = skip;
    dirty_ = true;
  }
}

void PickerScreen::drawHeader() {
  char text[48];
  switch (mode_) {
    case Mode::Main:      Format::period(text, sizeof text, view_.year, view_.semester); break;
    case Mode::Electives: snprintf(text, sizeof text, "%s", Strings::kElectivesFolder); break;
    case Mode::Years:     snprintf(text, sizeof text, "%s", Strings::kYearTitle); break;
    case Mode::ResetList: snprintf(text, sizeof text, "%s", Strings::kResetListTitle); break;
    default:              return;
  }
  header_.set(text);
  // Long press = favourite only exists for subjects, so the list of years does not mention it.
  bool simpleHint = (mode_ == Mode::Years || mode_ == Mode::ResetList);
  bool nothingToReset = (mode_ == Mode::ResetList && itemCount_ <= 1);   // only the Back row
  hint_.set(nothingToReset ? Strings::kResetNone : simpleHint ? Strings::kYearHint : Strings::kPickerHint);
}

void PickerScreen::describe(const Item& item, char* left, size_t leftSize, char* right, size_t rightSize,
                            RowLook& look) const {
  right[0] = '\0';
  look = RowLook();
  switch (item.kind) {
    case Item::Kind::Subject: {
      const SubjectStats& stats = Progress::stats(item.subjectIndex);
      snprintf(left, leftSize, "%s", Subjects::at(item.subjectIndex).name);
      if (stats.totalSeconds > 0) Format::duration(right, rightSize, stats.totalSeconds);
      if (stats.favorite) look = {true, Icon::Star, Colors::kWarning, TextField::IconSide::Before};
      break;
    }
    case Item::Kind::ElectivesFolder:
      snprintf(left, leftSize, "%s", Strings::kElectivesFolder);
      look = {true, Icon::Return, Colors::kDim, TextField::IconSide::Before};
      break;
    case Item::Kind::OtherPeriod:
      snprintf(left, leftSize, "%s", Strings::kOtherPeriod);
      look = {true, Icon::Return, Colors::kDim, TextField::IconSide::Before};
      break;
    case Item::Kind::YearChoice:
      snprintf(left, leftSize, Strings::kYearFormat, (unsigned)item.year);
      break;
    case Item::Kind::Back:
      snprintf(left, leftSize, "%s", Strings::kBack);
      look = {true, Icon::BackSmall, Colors::kDim, TextField::IconSide::Before};
      break;
  }
}

void PickerScreen::drawRows() {
  // A new focused row (or a new list) starts its sliding text from the beginning.
  if (marqueeSelected_ != selected_ || marqueeVersion_ != listVersion_) {
    marqueeSelected_ = selected_;
    marqueeVersion_ = listVersion_;
    marqueeStartMs_ = millis();
    marqueeSkip_ = 0;
  }

  for (uint8_t r = 0; r < kVisibleRows; r++) {
    size_t position = top_ + r;
    if (position >= itemCount_) {
      rowLeft_[r].clearIcon();
      rowLeft_[r].setScroll(-1);
      rowLeft_[r].setBounds(0, kScreenWidth);
      rowRight_[r].setBounds(kScreenWidth, 0);
      rowLeft_[r].set("", Colors::kText, Colors::kBackground);
      rowRight_[r].set("", Colors::kMuted, Colors::kBackground);
      continue;
    }
    char left[64], right[24];
    RowLook look;
    describe(items_[position], left, sizeof left, right, sizeof right, look);

    bool selected = (position == selected_);
    uint16_t background = selected ? Colors::kSelection : Colors::kBackground;

    // The study time gets only the width it needs; the rest of the row belongs to the name.
    int16_t rightWidth = right[0] ? Display::textWidth(Font::Small, right) + 2 * kRightPadding : 0;
    rowLeft_[r].setBounds(0, kScreenWidth - rightWidth);
    rowRight_[r].setBounds(kScreenWidth - rightWidth, rightWidth);

    if (look.hasIcon) rowLeft_[r].setIcon(look.icon, look.color, look.side);
    else rowLeft_[r].clearIcon();
    rowLeft_[r].setScroll(selected ? marqueeSkip_ : -1);   // only the focused row slides
    rowLeft_[r].set(left, Colors::kText, background);
    rowRight_[r].set(right, selected ? Colors::kText : Colors::kMuted, background);
  }
  arrows_.update(top_ > 0, top_ + kVisibleRows < itemCount_);
}
