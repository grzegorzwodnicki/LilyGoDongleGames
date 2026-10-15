#include "Button.h"

Button::Event Button::update(bool down, uint32_t longMs) {
  if (down && !down_) {
    down_ = true;
    consumed_ = false;
    downSinceMs_ = millis();
  }
  if (down_ && !consumed_ && down && millis() - downSinceMs_ >= longMs) {
    consumed_ = true;
    return Event::kLong;
  }
  if (!down && down_) {
    const bool wasShort = !consumed_;
    down_ = false;
    consumed_ = false;
    return wasShort ? Event::kShort : Event::kNone;
  }
  return Event::kNone;
}
