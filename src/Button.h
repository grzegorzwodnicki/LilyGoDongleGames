#pragma once

#include <Arduino.h>

// Tracks one button and tells short taps from long holds.
class Button {
 public:
  enum class Event : uint8_t { kNone, kShort, kLong };

  // kLong fires once, while the button is still down, after longMs.
  // kShort fires on release of a press shorter than longMs.
  Event update(bool down, uint32_t longMs);

  // True while pressed, but not after a long press already fired: the
  // rest of that hold is swallowed until the button is released.
  bool held() const { return down_ && !consumed_; }
  uint32_t heldMs() const { return held() ? millis() - downSinceMs_ : 0; }

 private:
  bool down_ = false;
  bool consumed_ = false;
  uint32_t downSinceMs_ = 0;
};
