# Agent Instructions — ST77916 LVGL Demo

This is an **ESP32-S3 embedded firmware project** using PlatformIO + Arduino framework + LVGL 8.3.
The target is an ST77916 QSPI 360×360 round display with CST816S capacitive touch.

## Build & Flash Commands

Always use PlatformIO CLI (`pio`) to build and upload. Never modify build artifacts directly.

| Task | Command |
|------|---------|
| Compile | `pio run` |
| Compile + upload | `pio run --target upload` |
| Serial monitor (115200) | `pio device monitor` |
| Clean build | `pio run --target clean` |
| List connected devices | `pio device list` |
| Verbose build | `pio run -v` |

After editing any `.c` / `.cpp` / `.h` file, always verify with `pio run` before considering the task complete.

## Platform Details

- **PlatformIO platform**: pioarduino `53.03.11` (Arduino ESP32 3.1.1 / IDF 5.3)
- **Board**: `esp32-s3-devkitc-1` — 16 MB flash, 8 MB PSRAM, 240 MHz
- **Upload baud**: 921600 | **Monitor baud**: 115200
- **Firmware output**: `.pio/build/esp32s3/firmware.bin`

## Manual Flash (esptool)

```bash
# Flash app partition only
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 .pio/build/esp32s3/firmware.bin

# Full flash (bootloader + partition table + app)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash \
  0x0000  .pio/build/esp32s3/bootloader.bin \
  0x8000  .pio/build/esp32s3/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/esp32s3/firmware.bin
```

If the device does not enter download mode automatically, hold **BOOT** while connecting USB, then release.

## Project Layout

```
hal/
  display.cpp     — display + touch + LVGL driver (ST77916 QSPI, CST816S I2C, LEDC backlight)
  display.h
  pincfg.h        — all GPIO pin defines
screens/
  screen_dashboard.c  — 3 concentric arc widgets, +/- buttons
  screen_info.c       — TabView: Online spinner / Calendar / Settings
  screen_image.c      — full-screen image
  screen_about.c      — designer credit label
  screen_agent.c      — Hex-Ball game (physics, custom draw, touch-to-rotate rings)
ui/
  ui.c / ui.h         — theme init, lazy screen loading, shared declarations
  ui_helpers.c / .h   — _ui_screen_change(), _ui_arc_increment()
  ui_img_*.c          — embedded image assets
lv_conf.h             — LVGL feature flags (only used widgets enabled)
platformio.ini        — build config; new .c files must be added to build_src_filter
main.cpp              — Arduino setup() / loop()
```

## Rules for Editing

1. **Each screen is one `.c` file** in `screens/`. Keep all static state inside that file.
2. **Declare new screens in `ui/ui.h`** — both the `_init()` function and `_get_ptr()` function.
3. **Register new source files** in `platformio.ini` under `build_src_filter` as `+<path/to/file.c>`.
4. **Never use global variables** across screen files — use the `get_ptr()` pattern for cross-screen references.
5. **LVGL draw callbacks** must check `lv_event_get_code(e) == LV_EVENT_DRAW_POST_BEGIN` before casting `lv_draw_ctx_t`.
6. **Timers** (e.g. game loop): create in `LV_EVENT_SCREEN_LOADED`, delete in `LV_EVENT_SCREEN_UNLOADED`.

## GPIO Pin Reference

| Signal | GPIO |
|--------|------|
| Backlight PWM | 15 |
| Display RST | 47 |
| Display CS | 10 |
| Display SCK | 9 |
| Display DATA0–3 | 11–14 |
| Touch SCL | 8 |
| Touch SDA | 7 |
| Touch INT | 41 |
| Touch RST | 40 |

## Common Pitfalls

- **`quad_mode` compile error** → wrong IDF version; confirm pioarduino platform is used, not official espressif32
- **`setup()` undefined** → `main.cpp` missing from `build_src_filter`
- **Screen navigation broken** → check that `_ui_screen_change` is called with the correct `get_ptr()` and `init` function for the target screen
- **Touch not registering** → check that `LV_OBJ_FLAG_CLICKABLE` is set and the object is not covered by a non-clickable transparent layer
