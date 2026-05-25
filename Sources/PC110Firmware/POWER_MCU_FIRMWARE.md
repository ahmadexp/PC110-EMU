# M3822x Power/Status MCU Firmware Notes

These notes describe the PC110 power/status MCU dump and the clean-room model used by the emulator. They are not a disassembly, translation, or reconstruction of RIOS microcontroller source.

## Identity

- ROM path: `Roms/M38223E4HP@QFP80.BIN`
- Desktop/source filename seen during inspection: `M38223E4HP@QFP80.BIN`
- Size: 16254 bytes
- SHA-256: `96c6e37cfa52f30b303db70c2036cbf21e6e1bb638c5eb11343ab161db3c9cc0`
- Banner string: `M3822X POWER SENSE MICON FIRMWARE Rev 8 (C) 1995 RIOS SYSTEMS CO.,LTD.`
- Parsed revision: `8`
- Emulator-reported model checksum from the clean-room loader: `FE951139`
- Hardware role in this emulator: power/status microcontroller, including the status/front-LCD family of behavior.

This was the dump that matched the PC110 front LCD/status controller family more closely than the keyboard MCU dump. The front LCD itself is currently represented by a host-side status model rather than by executing M3822x firmware instructions.

## Loader Behavior

`loadPowerSenseMCUFirmwareImage()` reads the binary and captures:

- `loaded`
- firmware ID/banner
- byte size
- model checksum
- parsed revision
- trailing bytes for diagnostic exposure

The loader accepts the ROM from default paths in the emulator core:

- `Roms/M38223E4HP@QFP80.BIN`
- `Roms/M3822X_POWER_SENSE_MICON.BIN`

Environment overrides:

- `PC110_MCU_FIRMWARE`
- `PC110_MCU_ROM`

## Clean-Room Model

The C++ model is `pc110::firmware::PowerSenseMCUModel`.

Tracked state:

- Firmware image metadata
- Selected indexed register
- Index read count
- Data read count
- Index write count
- Data write count

The model exposes firmware identity and useful status through the same indexed shape used by the emulator core.

## I/O Interface

The observed host interface is an indexed two-port register pair:

| Port | Direction | Meaning |
| --- | --- | --- |
| `00ECh` | read | Return current selected index. |
| `00ECh` | write | Select index. |
| `00EDh` | read | Read value for selected index. |
| `00EDh` | write | Store data byte at selected index in the emulator's indexed data array. |

The core traces this as the `Power-sense MCU indexed EC/ED` device.

## Indexed Register Map

The clean-room diagnostic map currently modeled by `PowerSenseMCUModel` is:

| Index range/value | Read behavior |
| --- | --- |
| `00h..7Fh` | Return stored indexed data value, defaulting to zero unless written. |
| `80h..DFh` | Firmware ID/banner bytes. |
| `E0h..EFh` | Tail bytes from the firmware image. |
| `F0h` | ASCII `M`. |
| `F1h` | ASCII `C`. |
| `F2h` | ASCII `U`. |
| `F3h` | Parsed firmware revision. |
| `F4h` | Firmware size low byte. |
| `F5h` | Firmware size middle byte. |
| `F6h` | Firmware size high byte. |
| `F7h` | Checksum byte 0. |
| `F8h` | Checksum byte 1. |
| `F9h` | Loaded/status marker `81h`. |
| `FAh` | Checksum byte 2. |
| `FBh` | Checksum byte 3. |
| `FCh` | Firmware ID length. |
| `FEh` | Data-read counter low byte. |
| `FFh` | Sentinel `A5h`. |

`FDh` is currently left as the stored indexed data value unless future traces identify a stronger meaning.

## Front LCD Relationship

The real PC110 front LCD is associated with the power/status controller family. Current emulator behavior separates this into two layers:

- The power MCU model exposes the M3822x firmware identity and indexed diagnostic behavior.
- `FrontLCDModel` renders visible status from host observations.

Current LCD status behavior:

- Startup logo: segmented `IBM` for 2.5 seconds after power-on/reset.
- Normal display: `HH:MM`.
- Detail text: boot media name or `idle`.
- Status indicators: BIOS hold, runtime, boot activity, power MCU revision, keyboard MCU loaded, disk, speaker, and setup.

This keeps the visible UI useful while leaving room for a future opcode-level M3822x emulator or a traced LCD protocol.

## Known Observations

- The firmware dump is short, around 16 KiB, consistent with a microcontroller firmware image rather than the 1 MiB font flash.
- The ASCII banner identifies RIOS Systems, 1995, revision 8.
- The emulator diagnostics show this as `M3822x power/status MCU`.
- The model reports the loaded firmware revision as `R8` in the front-LCD status indicator.

## Validation

Smoke target:

```sh
cmake --build build/cmake-portable --target pc110firmware-smoke
build/cmake-portable/pc110firmware-smoke
```

The smoke test loads `Roms/M38223E4HP@QFP80.BIN` and validates banner recognition, size, parsed revision, indexed metadata responses, signature bytes, and counters.

Expected smoke output shape:

```text
power MCU clean-room model ok: M3822X POWER SENSE MICON FIRMWARE Rev 8 (C) 1995 RIOS SYSTEMS CO.,LTD. size=16254 rev=8 checksum=FE951139
```

## Known Limits

- The model does not execute M3822x instructions.
- Battery, charging, lid, suspend, and power-event protocols are not fully traced yet.
- LCD segment drive timing is not modeled at the MCU pin level.
- The indexed diagnostic map is intentionally emulator-facing and may not be a literal hardware register map.
- Future hardware traces should replace placeholders with observed status bits.
