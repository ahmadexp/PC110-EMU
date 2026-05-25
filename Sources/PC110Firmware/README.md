# PC110 Firmware Models

This directory contains clean-room C++ behavior models for PC110 firmware-adjacent components.

These files are not disassembled or translated IBM/RIOS firmware source. They are compatibility models written from emulator observations, public PC BIOS conventions, ROM banner metadata, and the behavior already expressed in `PC110Core`.

## Power-Sense MCU Model

`PowerSenseMCUModel` models the observed M3822x power/status MCU interface used by the emulator:

- Firmware banner capture from `M38223E4HP@QFP80.BIN`.
- Revision parsing, currently matching the known `Rev 8` dump.
- Firmware size and checksum metadata.
- Indexed diagnostic register behavior:
  - `0x80...0xDF`: firmware ID bytes.
  - `0xE0...0xEF`: tail bytes from the firmware image.
  - `0xF0...0xF2`: `MCU` signature.
  - `0xF3`: firmware revision.
  - `0xF4...0xF6`: firmware size bytes.
  - `0xF7...0xF8`, `0xFA...0xFB`: checksum bytes.
  - `0xF9`: loaded/status marker.
  - `0xFC`: firmware ID length.
  - `0xFE`: data read counter low byte.
  - `0xFF`: sentinel `0xA5`.

The smoke test loads the real power MCU dump and validates the modeled responses:

```sh
cmake --build build/cmake-portable --target pc110firmware-smoke
build/cmake-portable/pc110firmware-smoke
```

## Keyboard MCU Model

`KeyboardControllerModel` models the observed MELPS 740 keyboard-controller side of the PC110 KBC path:

- Firmware banner capture from `M38813E4HP@QFP64.bin`.
- Version parsing, currently matching the known `Version 1.1` dump.
- Firmware size and checksum metadata.
- Stateful command byte handling.
- Observed 8042-style commands:
  - `0x20`: read command byte.
  - `0xAA`: controller self-test, queues `0x55`.
  - `0xAB`: keyboard interface test, queues `0x00`.
  - `0xA7` / `0xA8`: disable/enable aux.
  - `0xAD` / `0xAE`: disable/enable keyboard.
  - `0xD1`: mark output-port write phase.
  - `0xD4`: mark aux-device write phase.

The keyboard MCU smoke test loads the known dump and validates banner, version, command-byte state, queued responses, and counters:

```sh
cmake --build build/cmake-portable --target pc110kbdmcu-smoke
build/cmake-portable/pc110kbdmcu-smoke
```
