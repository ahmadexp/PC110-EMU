# PC110 Firmware Models

This directory contains clean-room C++ behavior models for PC110 firmware-adjacent components.

These files are not disassembled or translated IBM/RIOS firmware source. They are compatibility models written from emulator observations, public PC BIOS conventions, ROM banner metadata, and the behavior already expressed in `PC110Core`.

## Reference Notes

- [BIOS_FIRMWARE.md](BIOS_FIRMWARE.md): PC110 BIOS image identity, POST flow, BIOS interrupt services, Easy Setup, boot media, and front-LCD observations.
- [POWER_MCU_FIRMWARE.md](POWER_MCU_FIRMWARE.md): M3822x power/status MCU identity, indexed EC/ED interface, metadata registers, and front-LCD relationship.
- [KBD_MCU_FIRMWARE.md](KBD_MCU_FIRMWARE.md): MELPS 740 keyboard-controller MCU identity, 8042-style command behavior, KBC ports, and keyboard/aux state.

`MSM538032E@SOP44.BIN` is not one of the three control firmwares above. It is the PC110 Japanese font flash used by DBCS glyph rendering and is noted in the BIOS documentation where it affects display behavior.

## Smoke Tests

```sh
cmake --build build/cmake-portable --target pc110firmware-smoke
build/cmake-portable/pc110firmware-smoke

cmake --build build/cmake-portable --target pc110kbdmcu-smoke
build/cmake-portable/pc110kbdmcu-smoke

cmake --build build/cmake-portable --target pc110biosmodel-smoke
build/cmake-portable/pc110biosmodel-smoke
```
