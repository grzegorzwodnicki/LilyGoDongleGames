#include "Game.h"

#include <Preferences.h>

#include "Display.h"
#include "StatusLed.h"

namespace {
constexpr int kWidth = 160;
constexpr int kHeight = 80;
constexpr int kGroundY = 74;  // Top of the ground strip.

constexpr int kBirdX = 30;
constexpr int kBirdWidth = 10;
constexpr int kBirdHeight = 8;
constexpr float kGravity = 0.18f;
constexpr float kFlapVelocity = -2.6f;  // Rises ~19 px per flap.

constexpr int kPipeWidth = 16;
constexpr int kPipeSpacing = 90;  // Distance between pipe pairs.
constexpr float kPipeSpeed = 1.3f;
constexpr int kMinGap = 30;
constexpr int kStartGap = 42;
constexpr uint32_t kRestartDelayMs = 500;
constexpr uint32_t kIdleToTitleMs = 20000;  // Game over screen returns to the title.
constexpr uint32_t kDemoAfterMs = 20000;    // Idle title screen starts the demo.

constexpr uint16_t kSky = 0x04DF;
constexpr uint16_t kPipeGreen = 0x07E0;
constexpr uint16_t kPipeDark = 0x03E0;
constexpr uint16_t kGroundBrown = 0xA3A0;
constexpr char kPrefsNamespace[] = "flappy";

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

void Game::begin() {
  highScore_ = loadHighScore();
  titleSinceMs_ = millis();
  birdY_ = kHeight / 2 - kBirdHeight / 2;
}

int Game::gapHeight() const { return max(kMinGap, kStartGap - score_ / 5); }

void Game::reset(State next) {
  state_ = next;
  birdY_ = kHeight / 2 - kBirdHeight / 2;
  velocity_ = kFlapVelocity;  // The press that starts the game is the first flap.
  score_ = 0;
  pipeCount_ = 0;
  status_led::set(0, 0, 0);
}

void Game::showTitle() {
  state_ = State::kTitle;
  titleSinceMs_ = millis();
  pipeCount_ = 0;
  score_ = 0;
  status_led::set(0, 0, 0);
}

void Game::spawnPipe() {
  Pipe& pipe = pipes_[pipeCount_++];
  pipe.x = kWidth;
  pipe.gapTop = random(8, kGroundY - gapHeight() - 7);
  pipe.scored = false;
}

bool Game::collides() const {
  if (birdY_ < 0 || birdY_ + kBirdHeight > kGroundY) return true;
  const int left = kBirdX + 1;
  const int right = kBirdX + kBirdWidth - 1;
  const int top = static_cast<int>(birdY_) + 1;
  const int bottom = static_cast<int>(birdY_) + kBirdHeight - 1;
  for (int i = 0; i < pipeCount_; ++i) {
    const Pipe& p = pipes_[i];
    if (right > p.x && left < p.x + kPipeWidth &&
        (top < p.gapTop || bottom > p.gapTop + gapHeight())) {
      return true;
    }
  }
  return false;
}

void Game::gameOver() {
  state_ = State::kGameOver;
  diedAtMs_ = millis();
  status_led::set(255, 0, 0);
  if (score_ > highScore_) {
    highScore_ = score_;
    saveHighScore(highScore_);
  }
}

void Game::update(bool buttonDown) {
  const bool pressed = buttonDown && !wasDown_;
  wasDown_ = buttonDown;
  ++frame_;

  if (ledOffAtMs_ != 0 && millis() >= ledOffAtMs_) {
    ledOffAtMs_ = 0;
    status_led::set(0, 0, 0);
  }

  if (state_ == State::kTitle) {
    birdY_ = kHeight / 2 - kBirdHeight / 2 + 4 * sinf(frame_ * 0.1f);  // Idle bobbing.
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
    step(autopilotWantsFlap());
    return;
  }
  step(pressed);
}

// Aims for the bottom of the next gap and flaps whenever it starts to sink below that line.
bool Game::autopilotWantsFlap() const {
  int target = kGroundY / 2;  // Fly level until the first pipe shows up.
  for (int i = 0; i < pipeCount_; ++i) {
    if (pipes_[i].x + kPipeWidth > kBirdX - 2) {
      target = pipes_[i].gapTop + gapHeight() - 6;
      break;
    }
  }
  return velocity_ > 0 && birdY_ + kBirdHeight > target;
}

void Game::step(bool flap) {
  if (flap) velocity_ = kFlapVelocity;
  velocity_ += kGravity;
  birdY_ += velocity_;

  groundOffset_ += kPipeSpeed;
  for (int i = 0; i < pipeCount_; ++i) pipes_[i].x -= kPipeSpeed;
  while (pipeCount_ > 0 && pipes_[0].x + kPipeWidth < 0) {
    for (int i = 1; i < pipeCount_; ++i) pipes_[i - 1] = pipes_[i];
    --pipeCount_;
  }
  if (pipeCount_ == 0 || (pipeCount_ < kMaxPipes && pipes_[pipeCount_ - 1].x < kWidth - kPipeSpacing)) {
    spawnPipe();
  }

  for (int i = 0; i < pipeCount_; ++i) {
    if (!pipes_[i].scored && pipes_[i].x + kPipeWidth < kBirdX) {
      pipes_[i].scored = true;
      ++score_;
      if (state_ == State::kPlaying) {
        status_led::set(0, 255, 0);
        ledOffAtMs_ = millis() + 120;
      }
    }
  }

  if (collides()) {
    if (state_ == State::kDemo) {
      reset(State::kDemo);  // Demo just starts a fresh round.
    } else {
      gameOver();
    }
  }
}

void Game::draw() const {
  TFT_eSprite& c = display::canvas();
  c.fillSprite(kSky);

  // Pipes: a body with a wider cap at the gap edge.
  for (int i = 0; i < pipeCount_; ++i) {
    const Pipe& p = pipes_[i];
    const int x = static_cast<int>(p.x);
    const int gapBottom = p.gapTop + gapHeight();
    c.fillRect(x, 0, kPipeWidth, p.gapTop, kPipeGreen);
    c.fillRect(x - 2, p.gapTop - 5, kPipeWidth + 4, 5, kPipeDark);
    c.fillRect(x, gapBottom, kPipeWidth, kGroundY - gapBottom, kPipeGreen);
    c.fillRect(x - 2, gapBottom, kPipeWidth + 4, 5, kPipeDark);
  }

  // Ground with scrolling stripes.
  c.fillRect(0, kGroundY, kWidth, kHeight - kGroundY, kGroundBrown);
  const int offset = static_cast<int>(groundOffset_);
  for (int x = -(offset % 12); x < kWidth; x += 12) c.fillRect(x, kGroundY, 6, 2, TFT_DARKGREEN);

  // Bird: yellow body, white eye, orange beak, flapping wing.
  const int y = static_cast<int>(birdY_);
  const bool wingUp = state_ == State::kPlaying ? velocity_ < 0 : (frame_ / 8) % 2 == 0;
  c.fillRect(kBirdX, y, kBirdWidth - 2, kBirdHeight, TFT_YELLOW);
  c.fillRect(kBirdX + 6, y + 1, 3, 3, TFT_WHITE);
  c.fillRect(kBirdX + 8, y + 2, 1, 1, TFT_BLACK);
  c.fillRect(kBirdX + 8, y + 5, 3, 2, TFT_ORANGE);
  c.fillRect(kBirdX + 1, wingUp ? y + 1 : y + 4, 4, 3, TFT_ORANGE);

  if (state_ == State::kTitle) {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_WHITE);
    c.drawString("FLAPPY", kWidth / 2, 4, 4);
    c.setTextColor(TFT_BLACK);
    c.drawString("Press BOOT", kWidth / 2, 50, 2);
  } else {
    c.setTextDatum(TC_DATUM);
    c.setTextColor(TFT_WHITE);
    c.drawString(String(score_), kWidth / 2, 3, 4);
  }

  if (state_ == State::kDemo && (frame_ / 30) % 2 == 0) {
    c.setTextDatum(TL_DATUM);
    c.setTextColor(TFT_WHITE);
    c.drawString("DEMO", 3, 3, 2);
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
  display::present();
}
