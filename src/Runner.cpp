#include "Runner.h"

#include <Preferences.h>

#include "Display.h"
#include "StatusLed.h"

namespace {
constexpr int kWidth = 160;
constexpr int kGroundY = 66;
constexpr int kPlayerX = 20;
constexpr int kPlayerWidth = 10;
constexpr int kPlayerHeight = 14;

constexpr float kGravity = 0.5f;
constexpr float kJumpVelocity = 6.0f;     // Max jump height ~36 px.
constexpr float kReleaseVelocity = 3.0f;  // Letting go early cuts the jump short.
constexpr float kMinSpeed = 2.5f;
constexpr float kMaxSpeed = 6.0f;
constexpr int kMilestoneScore = 100;
constexpr uint32_t kRestartDelayMs = 500;
constexpr uint32_t kIdleToTitleMs = 20000;

constexpr uint16_t kCactusGreen = 0x07E0;
constexpr char kPrefsNamespace[] = "runner";

int loadHighScore() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);  // Creates the namespace on first run.
  const int value = prefs.getUInt("high", 0);
  prefs.end();
  return value;
}

void saveHighScore(int value) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putUInt("high", value);
  prefs.end();
}
}  // namespace

void RunnerGame::begin() { highScore_ = loadHighScore(); }

void RunnerGame::enter() { showTitle(); }

void RunnerGame::showTitle() {
  state_ = State::kTitle;
  obstacleCount_ = 0;
  score_ = 0;
  height_ = 0;
  status_led::set(0, 0, 0);
}

void RunnerGame::reset() {
  state_ = State::kPlaying;
  height_ = 0;
  velocity_ = 0;
  speed_ = kMinSpeed;
  scoreExact_ = 0;
  score_ = 0;
  milestone_ = 0;
  obstacleCount_ = 0;
  untilNextObstacle_ = 60;
  status_led::set(0, 0, 0);
}

void RunnerGame::spawnObstacle() {
  Obstacle& obstacle = obstacles_[obstacleCount_++];
  obstacle.x = kWidth;
  obstacle.width = random(6, 15);
  obstacle.height = random(10, 23);
  // Keep the gap longer than a full jump so every obstacle is clearable.
  untilNextObstacle_ = obstacle.width + 26 * speed_ + random(25, 71);
}

bool RunnerGame::collides() const {
  const int playerLeft = kPlayerX + 2;
  const int playerRight = kPlayerX + kPlayerWidth - 2;
  const int playerBottom = kGroundY - static_cast<int>(height_);
  const int playerTop = playerBottom - kPlayerHeight + 2;
  for (int i = 0; i < obstacleCount_; ++i) {
    const Obstacle& o = obstacles_[i];
    const bool overlapX = playerRight > o.x && playerLeft < o.x + o.width;
    const bool overlapY = playerBottom > kGroundY - o.height && playerTop < kGroundY;
    if (overlapX && overlapY) return true;
  }
  return false;
}

void RunnerGame::gameOver() {
  state_ = State::kGameOver;
  diedAtMs_ = millis();
  status_led::set(255, 0, 0);
  if (score_ > highScore_) {
    highScore_ = score_;
    saveHighScore(highScore_);
  }
}

void RunnerGame::update(bool buttonDown) {
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

  // Jump physics.
  if (pressed && height_ == 0) velocity_ = kJumpVelocity;
  if (!buttonDown && velocity_ > kReleaseVelocity) velocity_ = kReleaseVelocity;
  height_ += velocity_;
  velocity_ -= kGravity;
  if (height_ <= 0) {
    height_ = 0;
    velocity_ = 0;
  }

  // World scrolling and difficulty.
  speed_ = min(kMaxSpeed, kMinSpeed + score_ / 500.0f);
  groundOffset_ += speed_;
  for (int i = 0; i < obstacleCount_; ++i) obstacles_[i].x -= speed_;
  while (obstacleCount_ > 0 && obstacles_[0].x + obstacles_[0].width < 0) {
    for (int i = 1; i < obstacleCount_; ++i) obstacles_[i - 1] = obstacles_[i];
    --obstacleCount_;
  }
  untilNextObstacle_ -= speed_;
  if (untilNextObstacle_ <= 0 && obstacleCount_ < kMaxObstacles) spawnObstacle();

  // Score and milestone flash.
  scoreExact_ += speed_ * 0.08f;
  score_ = static_cast<int>(scoreExact_);
  if (score_ / kMilestoneScore > milestone_) {
    milestone_ = score_ / kMilestoneScore;
    status_led::set(0, 255, 0);
    ledOffAtMs_ = millis() + 150;
  }

  if (collides()) gameOver();
}

void RunnerGame::draw() const {
  TFT_eSprite& c = display::canvas();
  c.fillSprite(TFT_BLACK);

  // Ground with scrolling dashes.
  c.drawFastHLine(0, kGroundY + 1, kWidth, TFT_WHITE);
  const int offset = static_cast<int>(groundOffset_);
  for (int x = -(offset % 24); x < kWidth; x += 24) c.drawFastHLine(x, kGroundY + 4, 6, TFT_DARKGREY);

  // Obstacles: a cactus with two arms.
  for (int i = 0; i < obstacleCount_; ++i) {
    const Obstacle& o = obstacles_[i];
    const int x = static_cast<int>(o.x);
    c.fillRect(x, kGroundY - o.height, o.width, o.height, kCactusGreen);
    if (o.width >= 8) {
      c.fillRect(x - 2, kGroundY - o.height + 4, 2, 4, kCactusGreen);
      c.fillRect(x + o.width, kGroundY - o.height + 7, 2, 4, kCactusGreen);
    }
  }

  // Player, with a running leg animation on the ground.
  const int bottom = kGroundY - static_cast<int>(height_);
  const int top = bottom - kPlayerHeight;
  const bool legUp = height_ == 0 && state_ == State::kPlaying && (frame_ / 5) % 2 == 0;
  c.fillRect(kPlayerX, top, kPlayerWidth, kPlayerHeight - 4, TFT_WHITE);
  c.fillRect(kPlayerX + 6, top + 2, 2, 2, TFT_BLACK);
  c.fillRect(kPlayerX + 1, bottom - 4, 3, legUp ? 2 : 4, TFT_WHITE);
  c.fillRect(kPlayerX + 6, bottom - 4, 3, legUp ? 4 : 2, TFT_WHITE);

  if (state_ == State::kTitle) {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_WHITE);
    c.drawString("RUNNER", kWidth / 2 + 14, 6, 4);
    c.setTextColor(TFT_CYAN);
    c.drawString("Press BOOT", kWidth / 2 + 14, 38, 2);
    c.setTextDatum(TL_DATUM);
    c.setTextColor(TFT_DARKGREY);
    c.drawString("HI " + String(highScore_), 2, 2, 1);
    return;
  }

  // Score.
  c.setTextDatum(TR_DATUM);
  c.setTextColor(TFT_WHITE);
  c.drawString(String(score_), kWidth - 2, 2, 1);
  c.setTextDatum(TL_DATUM);
  c.setTextColor(TFT_DARKGREY);
  c.drawString("HI " + String(highScore_), 2, 2, 1);

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
