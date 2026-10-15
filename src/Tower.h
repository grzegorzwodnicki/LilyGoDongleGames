#pragma once

#include "GameBase.h"

// One-button Tower Stack. Tap to stop the sliding platform; whatever hangs
// over the platform below is cut off, so sloppy drops make the tower thinner.
class TowerGame : public GameBase {
 public:
  const char* name() const override { return "TOWER STACK"; }
  void begin() override;
  void enter() override;
  void update(bool buttonDown) override;
  void draw() const override;

 private:
  enum class State : uint8_t { kTitle, kPlaying, kGameOver };
  static constexpr int kMaxLevels = 250;

  void reset();
  void showTitle();
  void drop();
  void startMovingBlock();
  void cutChunk(int x, int width);
  void gameOver();
  float speed() const;

  State state_ = State::kTitle;
  bool wasDown_ = false;
  uint32_t frame_ = 0;
  uint32_t diedAtMs_ = 0;
  uint32_t ledOffAtMs_ = 0;

  uint8_t x_[kMaxLevels];  // Placed blocks, level 0 is the base.
  uint8_t w_[kMaxLevels];
  int level_ = 0;  // Level of the moving block (= number of placed blocks).
  float movingX_ = 0;
  int movingW_ = 0;
  int direction_ = 1;
  bool movingVisible_ = false;
  float camera_ = 0;  // Level shown at the bottom of the screen.

  // The cut-off piece that falls away.
  bool chunkActive_ = false;
  int chunkX_ = 0;
  int chunkW_ = 0;
  int chunkLevel_ = 0;
  float chunkFall_ = 0;
  float chunkVelocity_ = 0;

  int score_ = 0;
  int highScore_ = 0;
};
