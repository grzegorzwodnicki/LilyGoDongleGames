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
constexpr uint32_t kDemoAfterMs = 20000;   // Idle title screen starts the demo.
constexpr uint32_t kDemoRestartMs = 1500;  // Pause after a demo crash.
constexpr char kPrefsNamespace[] = "snake";
}  // namespace

void SnakeGame::begin() { highScore_ = highscore::load(kPrefsNamespace); }

void SnakeGame::enter() { showTitle(); }

void SnakeGame::showTitle() {
  state_ = State::kTitle;
  titleSinceMs_ = millis();
  status_led::set(0, 0, 0);
}

SnakeGame::Cell SnakeGame::bodyAt(int fromHead) const {
  return body_[(head_ - fromHead + kMaxLength) % kMaxLength];
}

void SnakeGame::reset(State next) {
  state_ = next;
  demoRestartAtMs_ = 0;
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
  if (state_ == State::kDemo) {  // The demo just waits a moment and starts over.
    demoRestartAtMs_ = millis() + kDemoRestartMs;
    return;
  }
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
    if (state_ == State::kPlaying) {
      status_led::set(0, 255, 0);
      ledOffAtMs_ = millis() + 120;
    }
    if (length_ == kMaxLength) {
      gameOver();  // The whole field is filled: nothing left to eat.
      return;
    }
    placeFood();
  }
}

// Breadth-first search to the food around the body (the tail cell counts as free,
// it moves away). Falls back to any free neighbour when the food is unreachable.
void SnakeGame::steerAutopilot() {
  constexpr int kCells = kCols * kRows;
  int16_t firstDirection[kCells];
  int16_t queue[kCells];
  bool visited[kCells] = {};

  const Cell head = bodyAt(0);
  const Cell tail = bodyAt(length_ - 1);
  const int start = head.y * kCols + head.x;
  int queueHead = 0;
  int queueTail = 0;
  visited[start] = true;
  queue[queueTail++] = start;

  int chosen = -1;
  while (queueHead < queueTail && chosen < 0) {
    const int current = queue[queueHead++];
    for (int d = 0; d < 4; ++d) {
      const int nx = (current % kCols + kDx[d] + kCols) % kCols;
      const int ny = (current / kCols + kDy[d] + kRows) % kRows;
      const int next = ny * kCols + nx;
      const bool isTail = nx == tail.x && ny == tail.y;
      if (visited[next] || (occupied_[ny][nx] && !isTail)) continue;
      visited[next] = true;
      firstDirection[next] = current == start ? d : firstDirection[current];
      if (nx == food_.x && ny == food_.y) {
        chosen = firstDirection[next];
        break;
      }
      queue[queueTail++] = next;
    }
  }

  if (chosen < 0) {
    for (int k = 0; k < 4 && chosen < 0; ++k) {
      const int d = (direction_ + k) % 4;
      const int nx = (head.x + kDx[d] + kCols) % kCols;
      const int ny = (head.y + kDy[d] + kRows) % kRows;
      if (!occupied_[ny][nx] || (nx == tail.x && ny == tail.y)) chosen = d;
    }
  }
  if (chosen >= 0) direction_ = chosen;
  pendingTurn_ = false;
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
    if (pressed) {
      reset(State::kPlaying);
    } else if (millis() - titleSinceMs_ > kDemoAfterMs) {
      reset(State::kDemo);
    }
    return;
  }
  if (state_ == State::kGameOver) {
    const uint32_t sinceDeath = millis() - diedAtMs_;
    if ((pressed && sinceDeath > kRestartDelayMs) || sinceDeath > kIdleToTitleMs) showTitle();
    return;
  }

  if (state_ == State::kDemo) {
    if (pressed) {
      showTitle();
      return;
    }
    if (demoRestartAtMs_ != 0) {
      if (millis() >= demoRestartAtMs_) reset(State::kDemo);
      return;
    }
  } else if (pressed) {
    pendingTurn_ = true;
  }
  if (++moveTimer_ >= framesPerMove()) {
    moveTimer_ = 0;
    if (state_ == State::kDemo) steerAutopilot();
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
  if (state_ == State::kDemo && (frame_ / 30) % 2 == 0) {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_WHITE);
    c.drawString("DEMO", kWidth / 2, 1, 1);
  }

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
