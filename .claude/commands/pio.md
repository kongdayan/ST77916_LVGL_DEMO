You are assisting with an ESP32-S3 firmware project that uses PlatformIO as the build system and LVGL 8.3 as the UI framework. The target hardware is an ST77916 QSPI 360×360 round display with CST816S capacitive touch.

When the user asks you to build, upload, monitor, debug, or inspect the firmware, use the following commands. Always prefer running them directly rather than asking the user to do it manually.

## Core PlatformIO Commands

| Task | Command |
|------|---------|
| Compile only | `pio run` |
| Compile + upload to connected device | `pio run --target upload` |
| Open serial monitor at 115200 baud | `pio device monitor` |
| Compile, upload, then monitor | `pio run --target upload && pio device monitor` |
| Clean all build artifacts | `pio run --target clean` |
| List all connected serial devices | `pio device list` |
| Show library dependency tree | `pio pkg list --environment esp32s3` |
| Update all libraries | `pio pkg update` |
| Run verbose build (shows compile errors in full) | `pio run -v` |

## Platform & Board

- **Platform**: pioarduino `53.03.11` — provides Arduino ESP32 3.1.1 / IDF 5.3
- **Board**: `esp32-s3-devkitc-1` (16 MB flash, 8 MB PSRAM)
- **Upload baud**: 921600
- **Monitor baud**: 115200
- **USB CDC**: enabled (`ARDUINO_USB_CDC_ON_BOOT=1`)

## Manual Flash with esptool (no PlatformIO needed)

```bash
# Flash a pre-built binary (e.g. from CI artifact)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 .pio/build/esp32s3/firmware.bin

# Flash all partitions (bootloader + partition table + app)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash \
  0x0000  .pio/build/esp32s3/bootloader.bin \
  0x8000  .pio/build/esp32s3/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/esp32s3/firmware.bin

# Read chip info
esptool.py --chip esp32s3 --port /dev/ttyUSB0 chip_id

# Erase entire flash (factory reset)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 erase_flash
```

## Serial Port Detection

- macOS: `/dev/cu.usbmodem*` (USB CDC) — PlatformIO auto-detects
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- Windows: `COM3`, `COM4`, etc.

If upload fails with "Failed to connect", hold the BOOT button on the ESP32-S3 DevKit while clicking Upload, then release after connection is established.

## Project Source Layout

```
hal/display.cpp        — LVGL + ST77916 + CST816S driver init
hal/display.h
hal/pincfg.h           — GPIO pin definitions
screens/screen_*.c     — One file per UI screen (Dashboard, Info, Image, About, Agent)
ui/ui.c / ui.h         — Theme init, first-screen load, shared declarations
ui/ui_helpers.c / .h   — _ui_screen_change(), _ui_arc_increment() helpers
lv_conf.h              — LVGL feature flags
platformio.ini         — Build config; add new .c files to build_src_filter here
```

## Adding a New Source File

Edit `platformio.ini` and add a line under `build_src_filter`:
```ini
+<screens/screen_newscreen.c>
```

## Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `No such file or directory: display.h` | Missing `-I${PROJECT_DIR}/hal` in build_flags | Add the `-I` flag to `platformio.ini` |
| `undefined reference to setup()` | `.ino` file not compiled | Ensure `+<main.cpp>` is in `build_src_filter` |
| `quad_mode has no member` | IDF 4.x used instead of 5.x | Check that pioarduino platform is active |
| `Failed to connect` | ESP32 not in download mode | Hold BOOT button while uploading |
