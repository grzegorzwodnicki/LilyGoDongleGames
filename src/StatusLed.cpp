#include "StatusLed.h"

namespace status_led {
namespace {
constexpr uint8_t kDataPin = 40;
constexpr uint8_t kClockPin = 39;
constexpr uint8_t kBrightness = 16;  // 0-31

void writeByte(uint8_t value) {
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(kDataPin, (value >> bit) & 1);
    digitalWrite(kClockPin, HIGH);
    digitalWrite(kClockPin, LOW);
  }
}
}  // namespace

void begin() {
  pinMode(kDataPin, OUTPUT);
  pinMode(kClockPin, OUTPUT);
  set(0, 0, 0);
}

void set(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < 4; ++i) writeByte(0x00);  // Start frame.
  writeByte(0xE0 | kBrightness);
  writeByte(blue);
  writeByte(green);
  writeByte(red);
  for (int i = 0; i < 4; ++i) writeByte(0xFF);  // End frame.
}

}  // namespace status_led
