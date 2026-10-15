#include "Snake.h"

#include "Display.h"
#include "HighScore.h"
#include "StatusLed.h"

namespace {
constexpr int kWidth = 160;
constexpr int kFieldTop = 10;  // Height of the score bar above the field.
constexpr int kStartLength = 3;
constexpr int kDx[] = {1, 0, -1, 0};
constexpr int kDy[] = {0, 1, 0, -1};
constexpr uint32_t kRestartDelayMs = 500;
constexpr uint32_t kIdleToTitleMs = 20000;
constexpr char kPrefsNamespace[] = "snake";
}  // namespace

void SnakeGame::begin() { highScore_ = highscore::load(kPrefsNamespace); }

void SnakeGame::enter() { showTitle(); }

void SnakeGame::showTitle() {
  state_ = State::kTitle;
  status_led::set(0, 0, 0);
}

SnakeGame::Cell SnakeGame::bodyAt(int fromHead) const {
  return body_[(head_ - fromHead + kMaxLength) % kMaxLength];
}

void SnakeGame::reset() {
  state_ = State::kPlaying;
  memset(occupied_, 0, sizeof(occupied_));
  length_ = kStartLength;
  head_ = kStartLength - 1;
  for (int i = 0; i < kStartLength; ++i) {
    body_[i] = {static_cast<int8_t>(10 + i), static_cast<int8_t>(kRows / 2)};
    occupied_[kRows / 2][10 + i] = true;
  }
  direction_ = 0;
  pendingTurn_ = false;
  moveTimer_ = 0;
  score_ = 0;
  placeFood();
  status_led::set(0, 0, 0);
}

void SnakeGame::placeFood() {
  do {
    food_ = {static_cast<int8_t>(random(kCols)), static_cast<int8_t>(random(kRows))};
  } while (occupied_[food_.y][food_.x]);
}

int SnakeGame::framesPerMove() const { return max(4, 9 - score_ / 4); }

void SnakeGame::gameOver() {
  state_ = State::kGameOver;
  diedAtMs_ = millis();
  status_led::set(255, 0, 0);
  if (score_ > highScore_) {
    highScore_ = score_;
    highscore::save(kPrefsNamespace, highScore_);
  }
}

void SnakeGame::move() {
  if (pendingTurn_) {
    direction_ = (direction_ + 1) % 4;
    pendingTurn_ = false;
  }
  const Cell head = bodyAt(0);
  const Cell next{static_cast<int8_t>((head.x + kDx[direction_] + kCols) % kCols),
                  static_cast<int8_t>((head.y + kDy[direction_] + kRows) % kRows)};
  const bool eats = next.x == food_.x && next.y == food_.y;
  if (!eats) {  // The tail moves away, so its cell is free to enter.
    const Cell tail = bodyAt(length_ - 1);
    occupied_[tail.y][tail.x] = false;
    --length_;
  }
  if (occupied_[next.y][next.x]) {
    gameOver();
    return;
  }
  head_ = (head_ + 1) % kMaxLength;
  body_[head_] = next;
  occupied_[next.y][next.x] = true;
  ++length_;
  if (eats) {
    score_ = length_ - kStartLength;
    status_led::set(0, 255, 0);
    ledOffAtMs_ = millis() + 120;
    if (length_ == kMaxLength) {
      gameOver();  // The whole field is filled: nothing left to eat.
      return;
    }
    placeFood();
  }
}

void SnakeGame::update(bool buttonDown) {
  const bool pressed = buttonDown && !wasDown_;
  wasDown_ = buttonDown;
  ++frame_;

  if (ledOffAtMs_ != 0 && millis() >= ledOffAtMs_) {
    ledOffAtMs_ = 0;
    status_led::set(0, 0, 0);
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

  if (pressed) pendingTurn_ = true;
  if (++moveTimer_ >= framesPerMove()) {
    moveTimer_ = 0;
    move();
  }
}

void SnakeGame::draw() const {
  TFT_eSprite& c = display::canvas();
  c.fillSprite(TFT_BLACK);

  if (state_ == State::kTitle) {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_GREEN);
    c.drawString("SNAKE", kWidth / 2, 4, 4);
    c.setTextColor(TFT_WHITE);
    c.drawString("Tap: turn right", kWidth / 2, 36, 2);
    c.setTextColor(TFT_CYAN);
    c.drawString("Press BOOT", kWidth / 2, 54, 2);
    return;
  }

  // Score bar.
  c.setTextDatum(TL_DATUM);
  c.setTextColor(TFT_WHITE);
  c.drawString("SCORE " + String(score_), 2, 1, 1);
  c.setTextDatum(TR_DATUM);
  c.setTextColor(TFT_DARKGREY);
  c.drawString("BEST " + String(highScore_), kWidth - 2, 1, 1);
  c.drawFastHLine(0, kFieldTop - 2, kWidth, TFT_DARKGREY);

  c.fillRect(food_.x * kCell, kFieldTop + food_.y * kCell, kCell - 1, kCell - 1, TFT_RED);
  for (int i = length_ - 1; i >= 0; --i) {
    const Cell cell = bodyAt(i);
    c.fillRect(cell.x * kCell, kFieldTop + cell.y * kCell, kCell - 1, kCell - 1, i == 0 ? TFT_YELLOW : TFT_GREEN);
  }

  if (state_ == State::kGameOver) {
    c.fillRect(4, 14, kWidth - 8, 52, TFT_BLACK);
    c.drawRect(4, 14, kWidth - 8, 52, TFT_RED);
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_RED);
    c.drawString("GAME OVER", kWidth / 2, 18, 4);
    c.setTextColor(TFT_WHITE);
    c.drawString("Score " + String(score_) + "  Best " + String(highScore_), kWidth / 2, 48, 2);
  }
}
