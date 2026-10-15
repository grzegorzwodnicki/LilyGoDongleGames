#pragma once

#include <TFT_eSPI.h>

// Double-buffered access to the T-Dongle S3 80x160 LCD: draw on canvas(),
// then call present() to push the finished frame to the panel.
namespace display {

void begin();
TFT_eSprite& canvas();
void present();

}  // namespace display
