#pragma once

#include "GameBase.h"

// One-button endless runner. Tap to jump, hold to jump higher.
class RunnerGame : public GameBase {
 public:
  const char* name() const override { return "RUNNER"; }
  void begin() override;
  void enter() override;
  void update(bool buttonDown) override;
  void draw() const override;

 private:
  enum class State : uint8_t { kTitle, kPlaying, kGameOver };
  struct Obstacle {
    float x;
    int width;
    int height;
  };
  static constexpr int kMaxObstacles = 3;

  void reset();
  void showTitle();
  void spawnObstacle();
  bool collides() const;
  void gameOver();

  State state_ = State::kTitle;
  bool wasDown_ = false;
  uint32_t frame_ = 0;
  uint32_t diedAtMs_ = 0;
  uint32_t ledOffAtMs_ = 0;

  float height_ = 0;    // Player height above the ground, px.
  float velocity_ = 0;  // Vertical speed, px/frame (up is positive).
  float speed_ = 0;     // Scroll speed, px/frame.
  float scoreExact_ = 0;
  float groundOffset_ = 0;
  float untilNextObstacle_ = 0;
  int score_ = 0;
  int highScore_ = 0;
  int milestone_ = 0;

  Obstacle obstacles_[kMaxObstacles];
  int obstacleCount_ = 0;
};
