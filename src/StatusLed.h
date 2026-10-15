#pragma once

#include <Arduino.h>

// Driver for the on-board APA102 RGB LED of the T-Dongle S3.
namespace status_led {

void begin();
void set(uint8_t red, uint8_t green, uint8_t blue);

}  // namespace status_led
