#pragma once

#include "GameBase.h"

// Game picker: tap to move the selection, hold to start the highlighted game.
class Menu {
 public:
  Menu(GameBase* const* games, int count) : games_(games), count_(count) {}

  void next() { selected_ = (selected_ + 1) % count_; }
  GameBase* selected() const { return games_[selected_]; }
  void draw(float holdProgress) const;  // holdProgress: 0..1 of the long press.

 private:
  GameBase* const* games_;
  int count_;
  int selected_ = 0;
};
