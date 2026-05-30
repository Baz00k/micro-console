# Micro Console

Tiny ESP32-C3 handheld console starter using PlatformIO, a Nokia 5110 / PCD8544 LCD, and four tactile buttons.

The firmware boots into the **Console Shell**, a simple bundled game selector currently titled `Micro Arcade`.

## Controls

In the **Console Shell**:

- `LEFT` / `RIGHT`: choose a bundled game.
- `A`: launch the selected bundled game.
- `B`: no action.

In a bundled game:

- Hold `B` for about 1 second to return to the **Console Shell**.

## Bundled Games

- `Snake`: wrap around Snake with body collision, food, score, and game-over retry.
- `Pong`: recreation of the classic Pong game.
- `Breakout`: clear a wall of bricks with a bouncing ball and paddle.
- `Button Test`: shows the last pressed button, frame count, and elapsed time.

## Hardware

Target board: `esp32-c3-devkitm-1`

Display: Nokia 5110 / PCD8544, 84x48 monochrome LCD.

Buttons: four tactile switches wired between GPIO and GND. Firmware enables ESP32 internal pullups, so no external pullup resistors are required for the first prototype.

## Wiring

| Nokia 5110 pin  | ESP32-C3 pin | Firmware name       |
| --------------- | ------------ | ------------------- |
| VCC             | 3V3          | -                   |
| GND             | GND          | -                   |
| CLK / SCLK      | GPIO4        | `LCD_CLK_PIN`       |
| DIN / DN / MOSI | GPIO6        | `LCD_DIN_PIN`       |
| DC / D-C        | GPIO7        | `LCD_DC_PIN`        |
| CE / CS / SCE   | GPIO21       | `LCD_CS_PIN`        |
| RST             | GPIO3        | `LCD_RESET_PIN`     |
| LIGHT / BL      | GPIO5        | `LCD_BACKLIGHT_PIN` |

| Button | ESP32-C3 pin | Wiring                        |
| ------ | ------------ | ----------------------------- |
| LEFT   | GPIO20       | Button between GPIO20 and GND |
| RIGHT  | GPIO8        | Button between GPIO8 and GND  |
| A      | GPIO9        | Button between GPIO9 and GND  |
| B      | GPIO10       | Button between GPIO10 and GND |

## Build And Upload

This project uses `mise` for a PlatformIO-compatible Python version. Install the Python package dependencies once:

```sh
mise exec -- python -m pip install -r requirements.txt
```

```sh
mise exec -- python -m platformio run
mise exec -- python -m platformio run --target upload
mise exec -- python -m platformio device monitor
```
