#pragma once

#include "GameBase.h"

// One-button Snake. Tap to turn clockwise; the field wraps around at the edges.
class SnakeGame : public GameBase {
 public:
  const char* name() const override { return "SNAKE"; }
  void begin() override;
  void enter() override;
  void update(bool buttonDown) override;
  void draw() const override;

 private:
  enum class State : uint8_t { kTitle, kPlaying, kGameOver };
  struct Cell {
    int8_t x;
    int8_t y;
  };
  static constexpr int kCols = 32;
  static constexpr int kRows = 14;  // The remaining 10 px on top show the score.
  static constexpr int kCell = 5;  // px per grid cell.
  static constexpr int kMaxLength = kCols * kRows;

  void reset();
  void showTitle();
  void move();
  void placeFood();
  void gameOver();
  int framesPerMove() const;
  Cell bodyAt(int fromHead) const;  // 0 = head.

  State state_ = State::kTitle;
  bool wasDown_ = false;
  bool pendingTurn_ = false;
  uint32_t frame_ = 0;
  uint32_t diedAtMs_ = 0;
  uint32_t ledOffAtMs_ = 0;

  Cell body_[kMaxLength];  // Ring buffer.
  bool occupied_[kRows][kCols];
  int head_ = 0;
  int length_ = 0;
  int direction_ = 0;  // 0 right, 1 down, 2 left, 3 up (clockwise).
  int moveTimer_ = 0;
  Cell food_{0, 0};
  int score_ = 0;
  int highScore_ = 0;
};
