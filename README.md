# Dongle Flappy

A one-button Flappy Bird clone for the **LilyGo T-Dongle S3** (ESP32-S3, 0.96" 80x160 ST7735 display, on-board APA102 RGB LED). The only input is the BOOT button.

## Gameplay

- **Press BOOT** to flap. The first press on the title screen starts the game.
- Fly through the gaps between pipes. Each pair you pass is one point.
- The gap narrows slowly as your score grows.
- The RGB LED flashes **green** on every point and turns **red** on game over.
- The high score is kept in flash (NVS) and survives power cycles.

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

- The game runs a fixed 60 Hz update loop (`main.cpp`) and renders into an off-screen `TFT_eSprite` that is pushed to the panel once per frame, so there is no flicker.
- `Game` is a small state machine (title, playing, game over) with simple AABB collision.
- Difficulty is tuned by the constants at the top of `src/Game.cpp` (gravity, flap strength, pipe speed, spacing, gap size).

## Project layout

```
src/main.cpp        fixed-timestep main loop and button input
src/Game.*          game state, physics, rendering
src/Display.*       double-buffered LCD access
src/StatusLed.*     APA102 LED driver (bit-banged)
platformio.ini      board, libraries and display configuration
```

## License

[MIT](LICENSE)
