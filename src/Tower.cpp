#include "Tower.h"

#include "Display.h"
#include "HighScore.h"
#include "StatusLed.h"

namespace {
constexpr int kWidth = 160;
constexpr int kHeight = 80;
constexpr int kBlockHeight = 7;
constexpr int kBaseX = 40;
constexpr int kBaseWidth = 80;
constexpr int kPerfectTolerance = 2;  // px; a near-perfect drop snaps and keeps the width.
constexpr int kVisibleLevelsAbove = 7;
constexpr uint32_t kRestartDelayMs = 500;
constexpr uint32_t kIdleToTitleMs = 20000;
constexpr char kPrefsNamespace[] = "tower";

constexpr uint16_t kPalette[] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA};
constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

int screenY(int level, float camera) {
  return kHeight - static_cast<int>((level - camera) * kBlockHeight) - kBlockHeight;
}
}  // namespace

void TowerGame::begin() { highScore_ = highscore::load(kPrefsNamespace); }

void TowerGame::enter() { showTitle(); }

void TowerGame::showTitle() {
  state_ = State::kTitle;
  movingVisible_ = false;
  chunkActive_ = false;
  level_ = 0;
  camera_ = 0;
  score_ = 0;
  status_led::set(0, 0, 0);
}

float TowerGame::speed() const { return min(3.5f, 1.2f + level_ * 0.04f); }

void TowerGame::startMovingBlock() {
  movingW_ = w_[level_ - 1];
  const bool fromLeft = level_ % 2 == 0;  // Alternate the side each level.
  movingX_ = fromLeft ? 0 : kWidth - movingW_;
  direction_ = fromLeft ? 1 : -1;
  movingVisible_ = true;
}

void TowerGame::reset() {
  state_ = State::kPlaying;
  x_[0] = kBaseX;
  w_[0] = kBaseWidth;
  level_ = 1;
  camera_ = 0;
  score_ = 0;
  chunkActive_ = false;
  status_led::set(0, 0, 0);
  startMovingBlock();
}

void TowerGame::cutChunk(int x, int width) {
  chunkActive_ = true;
  chunkX_ = x;
  chunkW_ = width;
  chunkLevel_ = level_;
  chunkFall_ = 0;
  chunkVelocity_ = 0;
}

void TowerGame::gameOver() {
  state_ = State::kGameOver;
  diedAtMs_ = millis();
  status_led::set(255, 0, 0);
  if (score_ > highScore_) {
    highScore_ = score_;
    highscore::save(kPrefsNamespace, highScore_);
  }
}

void TowerGame::drop() {
  const int px = x_[level_ - 1];
  const int pw = w_[level_ - 1];
  const int mx = static_cast<int>(movingX_ + 0.5f);
  const int mw = movingW_;
  const int left = max(mx, px);
  const int right = min(mx + mw, px + pw);
  movingVisible_ = false;

  if (right <= left) {  // Missed the tower completely.
    cutChunk(mx, mw);
    gameOver();
    return;
  }

  int newX = left;
  int newW = right - left;
  if (abs(mx - px) <= kPerfectTolerance) {
    newX = px;
    newW = pw;
    status_led::set(0, 255, 0);
    ledOffAtMs_ = millis() + 150;
  } else if (mx < px) {
    cutChunk(mx, px - mx);
  } else {
    cutChunk(px + pw, mx + mw - (px + pw));
  }

  x_[level_] = newX;
  w_[level_] = newW;
  ++level_;
  score_ = level_ - 1;
  if (level_ >= kMaxLevels) {
    gameOver();  // The tower reached the maximum height.
    return;
  }
  startMovingBlock();
}

void TowerGame::update(bool buttonDown) {
  const bool pressed = buttonDown && !wasDown_;
  wasDown_ = buttonDown;
  ++frame_;

  if (ledOffAtMs_ != 0 && millis() >= ledOffAtMs_) {
    ledOffAtMs_ = 0;
    status_led::set(0, 0, 0);
  }
  if (chunkActive_) {
    chunkVelocity_ += 0.4f;
    chunkFall_ += chunkVelocity_;
    if (chunkFall_ > kHeight) chunkActive_ = false;
  }

  if (state_ == State::kTitle) {
    if (pressed) reset();
    return;
  }
  if (state_ == State::kGameOver) {
    const uint32_t sinceDeath = millis() - diedAtMs_;
    if ((pressed && sinceDeath > kRestartDelayMs) || sinceDeath > kIdleToTitleMs) showTitle();
    return;
  }

  movingX_ += direction_ * speed();
  if (movingX_ <= 0) {
    movingX_ = 0;
    direction_ = 1;
  } else if (movingX_ + movingW_ >= kWidth) {
    movingX_ = kWidth - movingW_;
    direction_ = -1;
  }

  // The camera follows the tower upwards.
  const float target = max(0, level_ - kVisibleLevelsAbove);
  camera_ += (target - camera_) * 0.15f;

  if (pressed) drop();
}

void TowerGame::draw() const {
  TFT_eSprite& c = display::canvas();
  c.fillSprite(TFT_BLACK);

  if (state_ == State::kTitle) {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_YELLOW);
    c.drawString("STACK", kWidth / 2, 4, 4);
    c.setTextColor(TFT_WHITE);
    c.drawString("Tap: drop block", kWidth / 2, 36, 2);
    c.setTextColor(TFT_CYAN);
    c.drawString("Press BOOT", kWidth / 2, 54, 2);
    return;
  }

  const int firstLevel = max(0, static_cast<int>(camera_));
  for (int level = firstLevel; level < level_; ++level) {
    c.fillRect(x_[level], screenY(level, camera_), w_[level], kBlockHeight - 1,
               kPalette[level % kPaletteSize]);
  }
  if (movingVisible_) {
    c.fillRect(static_cast<int>(movingX_ + 0.5f), screenY(level_, camera_), movingW_, kBlockHeight - 1,
               kPalette[level_ % kPaletteSize]);
  }
  if (chunkActive_) {
    c.fillRect(chunkX_, screenY(chunkLevel_, camera_) + static_cast<int>(chunkFall_), chunkW_,
               kBlockHeight - 1, TFT_DARKGREY);
  }

  c.setTextDatum(TL_DATUM);
  c.setTextColor(TFT_WHITE);
  c.drawString(String(score_), 3, 2, 2);

  if (state_ == State::kGameOver) {
    c.fillRect(4, 14, kWidth - 8, 52, TFT_BLACK);
    c.drawRect(4, 14, kWidth - 8, 52, TFT_RED);
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_RED);
    c.drawString("GAME OVER", kWidth / 2, 18, 4);
    c.setTextColor(TFT_WHITE);
    c.drawString("Height " + String(score_) + "  Best " + String(highScore_), kWidth / 2, 48, 2);
  }
}
