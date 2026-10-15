#pragma once

#include <Arduino.h>

// One-button Flappy Bird clone. Press BOOT to flap, fly through the gaps.
class Game {
 public:
  void begin();
  void update(bool buttonDown);  // One fixed 60 Hz tick.
  void draw() const;

 private:
  enum class State : uint8_t { kTitle, kPlaying, kGameOver };
  struct Pipe {
    float x;
    int gapTop;  // Y of the top edge of the gap.
    bool scored;
  };
  static constexpr int kMaxPipes = 3;

  void reset();
  void spawnPipe();
  bool collides() const;
  void gameOver();
  int gapHeight() const;

  State state_ = State::kTitle;
  bool wasDown_ = false;
  uint32_t frame_ = 0;
  uint32_t diedAtMs_ = 0;
  uint32_t ledOffAtMs_ = 0;

  float birdY_ = 0;      // Top of the bird, px.
  float velocity_ = 0;   // Vertical speed, px/frame (down is positive).
  float groundOffset_ = 0;
  int score_ = 0;
  int highScore_ = 0;

  Pipe pipes_[kMaxPipes];
  int pipeCount_ = 0;
};
