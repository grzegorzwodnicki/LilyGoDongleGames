#include "Display.h"

namespace display {
namespace {
TFT_eSPI tft;
TFT_eSprite sprite(&tft);
}  // namespace

void begin() {
  tft.init();
  tft.setRotation(1);  // Landscape: 160x80.
  tft.fillScreen(TFT_BLACK);
  sprite.setColorDepth(16);
  sprite.createSprite(tft.width(), tft.height());
}

TFT_eSprite& canvas() { return sprite; }

void present() { sprite.pushSprite(0, 0); }

}  // namespace display
