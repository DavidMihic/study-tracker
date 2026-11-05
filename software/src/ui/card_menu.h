// A row of up to 4 "cards" (icon + label) that pops up at the bottom of a screen.
//   Turn  = move the selection     Click = choose the selected card
// If nothing is touched for Config::kMenuTimeoutMs it closes by itself and NOTHING happens, so a menu
// that was opened by accident (or forgotten) can never change anything.
// The menu draws only its own band at the bottom (kBandTop and below); the screen keeps what is above it.
#pragma once
#include <Arduino.h>
#include "drivers/input.h"
#include "ui/icons.h"

class CardMenu {
 public:
  static constexpr int16_t kBandTop = 168;    // everything from this y down belongs to the menu
  static constexpr uint8_t kMaxCards = 4;

  struct Card {
    Icons::Icon icon;
    const char* label;
  };

  enum class Result : uint8_t { None, Chosen, TimedOut };

  // Clears the band and draws the cards. The first card is selected.
  void open(const Card* cards, uint8_t count, uint32_t nowMs);

  // Clears the band again. The screen then redraws its own fields that were hidden under it.
  void close();

  bool isOpen() const { return open_; }

  // Feed it the events while it is open. Chosen: read chosen(). The menu stays open until close().
  Result onEvent(const Input::Event& event, uint32_t nowMs);

  // Call every loop while open: TimedOut when the time ran out.
  Result update(uint32_t nowMs);

  uint8_t chosen() const { return selected_; }

 private:
  void layout();
  void drawCard(uint8_t index);

  Card cards_[kMaxCards];
  uint8_t count_ = 0;
  uint8_t selected_ = 0;
  bool open_ = false;
  uint32_t lastInputMs_ = 0;
  int16_t cardWidth_ = 0, left_ = 0;
};
