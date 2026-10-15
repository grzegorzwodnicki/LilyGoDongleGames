#pragma once

#include <Arduino.h>

// Common interface of every game in the menu. Games only draw into
// display::canvas(); the main loop presents the finished frame.
class GameBase {
 public:
  virtual ~GameBase() = default;
  virtual const char* name() const = 0;
  virtual void begin() = 0;                  // One-time setup (e.g. load the high score).
  virtual void enter() = 0;                  // Called when the game is picked in the menu.
  virtual void update(bool buttonDown) = 0;  // One fixed 60 Hz tick.
  virtual void draw() const = 0;
};
