#include "Menu.h"

#include "Display.h"

namespace {
constexpr int kRowHeight = 16;
constexpr int kRowsTop = 2;
constexpr int kRowX = 4;
constexpr int kRowWidth = 152;
}  // namespace

void Menu::draw(float holdProgress) const {
  TFT_eSprite& c = display::canvas();
  c.fillSprite(TFT_BLACK);
  c.setTextDatum(ML_DATUM);

  for (int i = 0; i < count_; ++i) {
    const int y = kRowsTop + i * kRowHeight;
    const bool isSelected = i == selected_;
    if (isSelected) {
      c.fillRect(kRowX, y, static_cast<int>(kRowWidth * holdProgress), kRowHeight - 1, TFT_DARKGREEN);
      c.drawRect(kRowX, y, kRowWidth, kRowHeight - 1, TFT_WHITE);
    }
    c.setTextColor(isSelected ? TFT_WHITE : TFT_DARKGREY);
    c.drawString(games_[i]->name(), kRowX + 6, y + kRowHeight / 2, 2);
  }

  c.setTextDatum(BC_DATUM);
  c.setTextColor(TFT_CYAN);
  c.drawString("tap: next   hold: play", 80, 80, 1);
}
