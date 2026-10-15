#pragma once

#include <Arduino.h>

// Per-game high score stored in flash (NVS), keyed by a short namespace name.
namespace highscore {

int load(const char* key);
void save(const char* key, int value);

}  // namespace highscore
