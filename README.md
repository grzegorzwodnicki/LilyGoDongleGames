# LilyGoDongleGames

A tiny one-button game console for the **LilyGo T-Dongle S3** (ESP32-S3, 0.96" 80x160 ST7735 display, on-board APA102 RGB LED). The only input is the BOOT button.

## Controls

| Where       | Action                    | Result                                       |
|-------------|---------------------------|----------------------------------------------|
| Menu        | tap                       | select the next game                         |
| Menu        | hold 0.6 s                | start the highlighted game                   |
| In a game   | game specific (see below) | play                                         |
| In a game   | hold 2.5 s                | back to the menu (a bar shows the progress)  |

The button that started a game is ignored until you release it, so a game never begins with an accidental input.

## Games

### Flappy Bird

- Press BOOT to flap and fly through the gaps between pipes. Each pair you pass is one point.
- The gap narrows slowly as your score grows.
- After 20 s on the title screen an autopilot plays a demo; press BOOT to leave it.

### Runner

- Tap to jump, hold to jump higher. Obstacles speed up with your score.
- After 20 s on the title screen an autopilot plays a demo; press BOOT to leave it.

Both games flash the RGB LED **green** on points or milestones and turn it **red** on game over. After game over, press BOOT to return to the game's title screen (it also returns by itself after 20 s). High scores are stored per game in flash (NVS) and survive power cycles.

## Hardware

| Part    | Details                                                           |
|---------|-------------------------------------------------------------------|
| Board   | LilyGo T-Dongle S3, 16 MB flash                                   |
| Display | ST7735 80x160 over SPI (MOSI 3, SCLK 5, CS 4, DC 2, RST 1, BL 38) |
| LED     | APA102 (data 40, clock 39)                                        |
| Button  | BOOT (GPIO 0)                                                     |

## Build and flash

Requires [PlatformIO](https://platformio.org/).

```sh
pio run               # build
pio run -t upload     # flash
```

If the upload cannot connect, unplug the dongle, hold the **BOOT** button, plug it back in and release the button, then upload again.

Notes:

- The platform is pinned to `espressif32@6.9.0` (Arduino core 2.x). TFT_eSPI does not work reliably with core 3.x on the ESP32-S3.
- The display is configured entirely through `build_flags` in `platformio.ini`, including `USE_HSPI_PORT`, which is required on the ESP32-S3.

## How it works

- `main.cpp` runs a fixed 60 Hz loop. Games only draw into an off-screen `TFT_eSprite`; the loop pushes the finished frame to the panel once per frame, so there is no flicker.
- Every game implements the small `GameBase` interface (`name`, `begin`, `enter`, `update`, `draw`). Adding a game means writing one class and adding it to the `games` list in `main.cpp`; the menu adapts to the number of games.
- `Button` separates taps from holds and swallows the rest of a hold after a long press fired.
- Each game is a small state machine (title, playing, game over) with simple AABB collision. Difficulty is tuned by the constants at the top of each game's `.cpp`.

## Project layout

```
src/main.cpp        main loop, menu / game switching
src/GameBase.h      interface implemented by every game
src/Menu.*          game picker
src/Button.*        tap / hold detection
src/Flappy.*        Flappy Bird
src/Runner.*        Runner
src/Display.*       double-buffered LCD access
src/StatusLed.*     APA102 LED driver (bit-banged)
platformio.ini      board, libraries and display configuration
```

## License

[MIT](LICENSE)
