#include <Arduino.h>

#include "Button.h"
#include "Display.h"
#include "Flappy.h"
#include "GameBase.h"
#include "Menu.h"
#include "Runner.h"
#include "Snake.h"
#include "Tower.h"
#include "StatusLed.h"

namespace {
constexpr uint8_t kButtonPin = 0;  // BOOT button, active low.
constexpr uint32_t kFrameMs = 16;  // ~60 fps.
constexpr uint32_t kSelectHoldMs = 600;  // Menu: hold to start the highlighted game.
constexpr uint32_t kExitHoldMs = 2500;   // In a game: hold to return to the menu.
constexpr uint32_t kExitHintMs = 1000;   // Show the exit progress bar after this long.

FlappyGame flappy;
RunnerGame runner;
SnakeGame snake;
TowerGame tower;
GameBase* const games[] = {&flappy, &runner, &snake, &tower};
constexpr int kGameCount = sizeof(games) / sizeof(games[0]);

Menu menu(games, kGameCount);
Button button;
GameBase* active = nullptr;  // nullptr while the menu is shown.
uint32_t lastFrameMs = 0;

void tickMenu(bool down) {
  const Button::Event event = button.update(down, kSelectHoldMs);
  if (event == Button::Event::kShort) {
    menu.next();
  } else if (event == Button::Event::kLong) {
    active = menu.selected();
    active->enter();
    return;
  }
  menu.draw(min(1.0f, static_cast<float>(button.heldMs()) / kSelectHoldMs));
}

void tickGame(bool down) {
  if (button.update(down, kExitHoldMs) == Button::Event::kLong) {
    active = nullptr;
    status_led::set(0, 0, 0);
    return;
  }
  active->update(button.held());
  active->draw();

  // Progress bar along the bottom edge while holding to exit.
  const uint32_t held = button.heldMs();
  if (held > kExitHintMs) {
    const int width = 160 * (held - kExitHintMs) / (kExitHoldMs - kExitHintMs);
    display::canvas().fillRect(0, 78, width, 2, TFT_WHITE);
  }
}
}  // namespace

void setup() {
  pinMode(kButtonPin, INPUT_PULLUP);
  status_led::begin();
  display::begin();
  for (GameBase* game : games) game->begin();
}

void loop() {
  if (millis() - lastFrameMs < kFrameMs) return;
  lastFrameMs = millis();

  const bool down = digitalRead(kButtonPin) == LOW;
  if (active == nullptr) {
    tickMenu(down);
  } else {
    tickGame(down);
  }
  display::present();
}
