#include "ui/card_menu.h"
#include "config/config.h"
#include "drivers/buzzer.h"
#include "drivers/display.h"
#include "ui/colors.h"

namespace {

constexpr int16_t kCardHeight = 64;
constexpr int16_t kCardTop = CardMenu::kBandTop + 4;
constexpr int16_t kGap = 8;
constexpr int16_t kMaxCardWidth = 96;

}  // namespace

void CardMenu::layout() {
  int16_t room = Config::kScreenWidth - 2 * kGap;
  cardWidth_ = (room - (count_ - 1) * kGap) / count_;
  if (cardWidth_ > kMaxCardWidth) cardWidth_ = kMaxCardWidth;
  int16_t total = count_ * cardWidth_ + (count_ - 1) * kGap;
  left_ = (Config::kScreenWidth - total) / 2;
}

void CardMenu::open(const Card* cards, uint8_t count, uint32_t nowMs) {
  if (count > kMaxCards) count = kMaxCards;
  count_ = count;
  for (uint8_t i = 0; i < count_; i++) cards_[i] = cards[i];
  selected_ = 0;
  open_ = true;
  lastInputMs_ = nowMs;

  Display::tft().fillRect(0, kBandTop, Config::kScreenWidth, Config::kScreenHeight - kBandTop,
                          Colors::kBackground);
  layout();
  for (uint8_t i = 0; i < count_; i++) drawCard(i);
}

void CardMenu::close() {
  if (!open_) return;
  open_ = false;
  Display::tft().fillRect(0, kBandTop, Config::kScreenWidth, Config::kScreenHeight - kBandTop,
                          Colors::kBackground);
}

void CardMenu::drawCard(uint8_t index) {
  Adafruit_ST7789& t = Display::tft();
  bool selected = (index == selected_);
  int16_t x = left_ + index * (cardWidth_ + kGap);
  uint16_t fill = selected ? Colors::kSelection : Colors::kCard;
  uint16_t ink = selected ? Colors::kText : Colors::kMuted;

  t.fillRoundRect(x, kCardTop, cardWidth_, kCardHeight, 8, fill);
  // The border is always drawn (in the fill colour when not selected) so the card never changes size.
  t.drawRoundRect(x, kCardTop, cardWidth_, kCardHeight, 8, selected ? Colors::kAccent : fill);
  t.drawRoundRect(x + 1, kCardTop + 1, cardWidth_ - 2, kCardHeight - 2, 7, selected ? Colors::kAccent : fill);

  Icons::draw(cards_[index].icon, x + cardWidth_ / 2, kCardTop + 24, ink, fill);

  const char* label = cards_[index].label;
  int16_t width = Display::textWidth(Display::Font::Small, label);
  int16_t baseline = kCardTop + kCardHeight - 8 + Display::fontDescent(Display::Font::Small);
  Display::drawText(Display::Font::Small, x + (cardWidth_ - width) / 2, baseline, label, ink);
}

CardMenu::Result CardMenu::onEvent(const Input::Event& event, uint32_t nowMs) {
  if (!open_) return Result::None;
  lastInputMs_ = nowMs;

  if (event.type == Input::Event::Type::Turn) {
    int target = (int)selected_ + event.delta;
    if (target >= 0 && target < (int)count_) {
      uint8_t old = selected_;
      selected_ = (uint8_t)target;
      drawCard(old);          // only the two cards that changed
      drawCard(selected_);
      Buzzer::play(Buzzer::Pattern::Click);
    }
    return Result::None;
  }
  if (event.type == Input::Event::Type::Click) return Result::Chosen;
  return Result::None;        // a long press does nothing here
}

CardMenu::Result CardMenu::update(uint32_t nowMs) {
  if (open_ && nowMs - lastInputMs_ >= Config::kMenuTimeoutMs) return Result::TimedOut;
  return Result::None;
}
