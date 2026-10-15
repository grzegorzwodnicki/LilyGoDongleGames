#include <Arduino.h>

#include "Display.h"
#include "Game.h"
#include "StatusLed.h"

namespace {
constexpr uint8_t kButtonPin = 0;  // BOOT button, active low.
constexpr uint32_t kFrameMs = 16;  // ~60 fps.

Game game;
uint32_t lastFrameMs = 0;
}  // namespace

void setup() {
  pinMode(kButtonPin, INPUT_PULLUP);
  status_led::begin();
  display::begin();
  game.begin();
}

void loop() {
  if (millis() - lastFrameMs < kFrameMs) return;
  lastFrameMs = millis();

  game.update(digitalRead(kButtonPin) == LOW);
  game.draw();
}
