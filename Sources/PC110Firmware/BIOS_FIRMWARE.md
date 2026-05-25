# PC110 BIOS Firmware Notes

These notes describe the PC110 BIOS behavior model used by the emulator. They are not a disassembly, translation, or reconstruction of IBM BIOS source. The model is a clean-room compatibility layer built from ROM identity checks, emulator traces, public PC BIOS conventions, and the behavior needed by the current PC110 bring-up path.

## Identity

- ROM path: `Roms/pc110_bios.bin`
- Original uploaded dump name: `E28F002BXT@TSOP40.BIN`
- Size: 262144 bytes / 256 KiB
- SHA-256: `232101c88466f311bcc32fbc215a4d7569f695ce19f9c07ca67ce2aee5232312`
- MD5 from inspection notes: `6de4281a58509438a2773365ba6b1371`
- Top reset-vector bytes at ROM offset `0x3FFF0`: `EA 5B E0 00 F0 31 31 2F 30 38 2F 39 35 00 FC B0`
- Decoded first instruction: far jump to `F000:E05B`
- Conventional reset-vector address: `000F:FFF0`
- 486 reset alias address: `0xFFFFFFF0`
- Target linear address: `0x000FE05B`
- Target offset inside the 256 KiB image: `0x3E05B`

Selected strings already found in the ROM:

- `09/19/95'(C) Copyright IBM Corp. 1994, All Rights Reserved.`
- `IBM VGA Compatible BIOS. 4`
- `a8Chips 65535 VGA 32KB BIOS`
- `Version 2.0.2`
- `APM BIOS 1.00.27`
- `(C) COPYRIGHT RIOS SYSTEMS, 1993, 1994. ALL RIGHTS RESERVED.`
- `39H4551 (C) COPYRIGHT IBM CORPORATION 1981, 1995 ALL RIGHTS RESERVED 11/08/953`
- `COPR. IBM 1981, 1995`

ROM signature bytes `55 AA` were seen on 16-byte boundaries at offsets `0x00000`, `0x20000`, `0x26F90`, `0x27B20`, `0x28750`, `0x2E740`, and `0x35FC0`.

## Related ROM Assets

The BIOS is the executable system firmware. The separate `Roms/MSM538032E@SOP44.BIN` image is a Japanese font flash:

- Size: 1048576 bytes / 1 MiB
- SHA-256: `9829fdb8281c12022dc3b77686044ed1a5213ab526ce4329f2841cd64171784c`
- Early visible strings include `FONT`, `84G7940`, and `03/23/95`
- Emulator role: DBCS/Japanese glyph source for ROM-backed rendering paths

## Clean-Room Model

The C++ model is `pc110::firmware::PC110BiosModel` in `PC110FirmwareModel.cpp` and `PC110FirmwareModel.hpp`. It captures the BIOS-facing contract instead of executing or translating ROM instructions.

Tracked BIOS state:

- BIOS loaded flag
- POST phase
- Easy Setup request flag
- A20 enabled state
- Base and extended memory sizes
- Host seconds since power-on
- RTC date and time
- Attached boot media geometry
- BIOS keyboard queue

The model starts with 640 KiB base memory, 15360 KiB extended memory, A20 enabled, and an empty boot-media slot.

## POST And Boot Flow

The modeled POST phases are:

1. `ResetVector`
2. `Post`
3. `OptionRom`
4. `BootStrap`
5. `Runtime`
6. `EasySetup`

`advancePOST()` moves from reset vector through POST and option-ROM setup. If Easy Setup has been requested, the option-ROM phase diverts to `EasySetup`; otherwise it moves through `BootStrap` to `Runtime`.

The runtime phase is used as the host observation that the machine is in continuous run. The bootstrap phase is used as the host observation that visual boot activity is happening.

## Easy Setup

The Easy Setup model tracks the PC110 setup shell at a functional level:

- Pages: `Config`, `Date/Time`, `Password`, `Start up`, `Test`, `Restart`
- A selected page
- Detail-panel state
- Restart-confirm state

The emulator can enter this path either by explicitly requesting setup before POST completes or by host-side setup controls that inject the BIOS setup path.

## INT 13h Disk Services

The BIOS service model handles the disk calls required by current boot and diagnostics:

| Request | Behavior |
| --- | --- |
| `AH=00h` | Reset succeeds when boot media is attached. |
| `AH=02h` | Read succeeds when `AL` is nonzero and media is attached; zero-sector reads fail with carry set. |
| `AH=08h` | Returns attached disk geometry. |
| `AH=15h` | Reports DASD type `03h` when media is attached. |
| Other functions | Fail with carry set. |

The attached geometry defaults to 80 cylinders, 2 heads, 18 sectors per track, BIOS drive `00h`. When a disk image is attached, the model reports last cylinder, last head, sectors per track, and drive number in the conventional BIOS register fields.

## INT 15h Services

Modeled services:

| Request | Behavior |
| --- | --- |
| `AX=2101h` | Accepted as a PC110 private service. |
| `AX=2400h` | Disables A20. |
| `AX=2401h` | Enables A20. |
| `AX=2402h` | Reports current A20 state. |
| `AX=2403h` | Reports keyboard-controller and fast A20 support. |
| `AX=E801h` | Returns extended-memory geometry. |
| `AH=88h` | Reports extended memory below 16 MiB. |
| `AH=C0h` | Returns a system-configuration table pointer placeholder. |
| `AX=5000h` / `AX=5380h` | Modeled as PC110 private wait/event services with no event pending, carry set, and timeout-style status. |
| Other requests | Benign success unless a more specific behavior is added. |

The A20 state is shared with the BIOS model, so `AX=2400h`, `AX=2401h`, and `AX=2402h` are stateful.

## INT 16h Keyboard Services

The model has a BIOS keyboard queue of `{ascii, scan}` pairs.

| Request | Behavior |
| --- | --- |
| `AH=00h` / `AH=10h` | Reads and consumes one queued key. Empty queue returns zero flag set. |
| `AH=01h` / `AH=11h` | Peeks one queued key. Empty queue returns zero flag set. |
| `AH=02h` / `AH=12h` | Returns clear shift flags. |
| `AH=05h` | Stores the key supplied in `CX`. |
| Other requests | Empty/no-key response. |

The return convention is `AH=scancode`, `AL=ASCII`.

## INT 1Ah RTC Services

The RTC model supports:

- `AH=00h`: BIOS tick count derived from seconds since power-on at approximately 18.2065 ticks per second.
- `AH=02h`: time-of-day in BCD.
- `AH=04h`: date in BCD.

The stateful model defaults to date `2026-05-24` and lets the caller set date and clock values explicitly.

## Front LCD Observation

`PC110BiosModel::frontLCD()` renders through `FrontLCDModel`.

Current LCD behavior:

- First 2.5 seconds after power-on: startup logo mode with segmented `IBM`.
- After startup: status mode with `HH:MM`.
- Detail text: active boot-media name or `idle`.
- Indicators:
  - `HOLD`: no BIOS loaded
  - `RUN`: continuous runtime
  - `BOOT`: visual boot phase
  - `PMCU R8`: power MCU loaded, revision 8 in the modeled observation
  - `KBC`: keyboard MCU loaded
  - `DSK`: boot media attached
  - `SND`: speaker active
  - `SET`: Easy Setup active

The segmented `IBM` startup rendering was added after comparing the PC110 front-LCD reference photo where the real unit writes IBM on startup using LCD segments.

## Emulator Integration Notes

The full core still executes the ROM image and contains bring-up scaffolding around observed BIOS paths. Important mapped behavior from `PC110Core` includes:

- BIOS image mapped into the conventional top-of-1-MiB BIOS region.
- A reset alias mapped at the 486 top-of-address-space reset vector.
- F000 shadow behavior that keeps the BIOS region writable where the PC110 ROM expects shadow RAM.
- ROM-backed BIOS Easy Setup rendering.
- INT-service compatibility paths for disk, keyboard, timer/RTC, serial, memory, and private PC110 services.
- Host controls for BIOS setup, reset, run/pause, boot disk selection, and diagnostics.

## Validation

Smoke target:

```sh
cmake --build build/cmake-portable --target pc110biosmodel-smoke
build/cmake-portable/pc110biosmodel-smoke
```

The smoke test checks POST phase transitions, startup LCD `IBM`, disk geometry, zero-sector read failure, keyboard queue behavior, A20 state changes, RTC time/date services, and Easy Setup LCD state.

## Known Limits

- This is not a CPU-level BIOS reimplementation.
- Many ROM functions are still represented as compatibility behavior rather than exact firmware execution.
- The system-configuration table pointer is a placeholder.
- Private PC110 BIOS calls are modeled only where traces identified useful behavior.
- Date/time behavior is deterministic unless the host sets it.
- The model intentionally avoids proprietary source reconstruction.
