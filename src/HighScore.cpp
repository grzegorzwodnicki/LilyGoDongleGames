#include "HighScore.h"

#include <Preferences.h>

namespace highscore {

int load(const char* key) {
  Preferences prefs;
  prefs.begin(key, /*readOnly=*/false);  // Creates the namespace on first run.
  const int value = prefs.getUInt("high", 0);
  prefs.end();
  return value;
}

void save(const char* key, int value) {
  Preferences prefs;
  prefs.begin(key, /*readOnly=*/false);
  prefs.putUInt("high", value);
  prefs.end();
}

}  // namespace highscore
