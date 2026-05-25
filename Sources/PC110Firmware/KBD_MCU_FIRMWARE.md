# MELPS 740 Keyboard MCU Firmware Notes

These notes describe the PC110 keyboard-controller MCU dump and the clean-room model used by the emulator. They are not a disassembly, translation, or reconstruction of RIOS microcontroller source.

## Identity

- ROM path: `Roms/M38813E4HP@QFP64.bin`
- Desktop/source filename seen during inspection: `M38813E4HP@QFP64.bin`
- Size: 16255 bytes
- SHA-256: `b29761a2fd39abd9c9419ca73b03beb6c41bc52102e2a7429d1f023f82a2a2b8`
- Banner string: `MELPS 740 Series Keyboard Firmware Version 1.1(C) Copyright 1992-1995 RIOS Systems Co.,Ltd.`
- Parsed version: `1.1`
- Emulator-reported model checksum from the clean-room loader: `8FC1D1C6`
- Hardware role in this emulator: keyboard-controller MCU/KBC behavior.

This dump was initially considered as a possible front-LCD controller, but the embedded banner and observed behavior identify it as the keyboard-controller firmware.

## Loader Behavior

`loadKeyboardControllerMCUFirmwareImage()` reads the binary and captures:

- `loaded`
- firmware ID/banner
- byte size
- model checksum
- parsed major/minor version
- trailing bytes for diagnostic exposure

The emulator core searches for the ROM at:

- `Roms/M38813E4HP@QFP64.bin`
- `Roms/M38813E4HP@QFP64.BIN`

Environment overrides:

- `PC110_KEYBOARD_MCU_FIRMWARE`
- `PC110_KEYBOARD_MCU_ROM`

## Clean-Room Model

The C++ model is `pc110::firmware::KeyboardControllerModel`.

Tracked state:

- Firmware image metadata
- 8042 command byte, default `45h`
- Pending controller response byte
- Command count
- Response-read count
- Self-test count
- Interface-test count
- Keyboard disabled flag
- Auxiliary disabled flag

The model is stateful enough for BIOS POST, keyboard diagnostics, Easy Setup entry, and host diagnostics.

## I/O Interface

The emulator core maps the keyboard/system controller through conventional PC/AT-style ports:

| Port | Direction | Meaning |
| --- | --- | --- |
| `0060h` | read | KBC data/output byte. |
| `0060h` | write | KBC data byte, command-byte payload, output-port payload, or auxiliary-device payload depending on pending command. |
| `0064h` | read | KBC status. |
| `0064h` | write | KBC command. |
| `0061h` | read/write | System control port with timer/speaker status behavior. |

Important status behavior:

- `0064h` bit 0 is set when a controller response is ready.
- Auxiliary PS/2 response paths report output-buffer-full plus auxiliary data indication.
- The BIOS self-test helper expects a ready `55h` byte from port `0060h`.

## Modeled Commands

`KeyboardControllerModel::command()` currently models these 8042-style commands:

| Command | Behavior |
| --- | --- |
| `20h` | Queue current command byte for the next data read. |
| `AAh` | Controller self-test; queue `55h`; increment self-test counter. |
| `ABh` | Keyboard interface test; queue `00h`; increment interface-test counter. |
| `A7h` | Disable auxiliary interface; set command-byte bit 5. |
| `A8h` | Enable auxiliary interface; clear command-byte bit 5. |
| `A9h` | Auxiliary interface test; queue `00h`; increment interface-test counter. |
| `ADh` | Disable keyboard; set command-byte bit 4. |
| `AEh` | Enable keyboard; clear command-byte bit 4. |
| `D1h` | Mark next data byte as output-port write phase. |
| `D4h` | Mark next data byte as auxiliary-device write phase. |

The full core also handles:

| Command | Core behavior |
| --- | --- |
| `60h` | Next `0060h` data write updates the command byte. |
| `D0h` | Next `0060h` data read returns the output port. |
| `FEh` | CPU reset request. |

## Command Byte And Flags

The command byte defaults to `45h`.

Modeled flag relationships:

- Bit 4 set: keyboard disabled.
- Bit 5 set: auxiliary interface disabled.

Writing the command byte through command `60h` in the full core updates both disabled flags. The clean-room C++ model exposes `writeCommandByte()` directly for tests and diagnostics.

## Data Responses

Current controller response behavior:

- Self-test returns `55h`.
- Keyboard interface test returns `00h`.
- Auxiliary interface test returns `00h`.
- Read-command-byte returns the current command byte.

Auxiliary-device writes in the full core queue PS/2-style responses:

- Any auxiliary command returns `FAh` ACK.
- Auxiliary reset command `FFh` returns `FAh`, `AAh`, `00h`.

## BIOS And Input Path Relationship

The keyboard MCU model supports the low-level controller side. The BIOS model separately handles `INT 16h` at the BIOS service layer:

- Queued keys return as `AH=scancode`, `AL=ASCII`.
- Empty reads/peeks set zero flag.
- Stored keys are accepted through `AH=05h`.

The full emulator bridges host keyboard input into BIOS-visible events and also has a setup path that can provide the F1 make scancode `3Bh` when real BIOS Easy Setup is requested.

## Known Observations

- The firmware banner explicitly identifies a MELPS 740 keyboard firmware.
- Version is `1.1`.
- Copyright text covers 1992-1995 RIOS Systems.
- The dump is roughly 16 KiB, similar in size to the power MCU firmware but with a different banner and role.
- POST expects the controller self-test byte `55h`; returning `00h` sends the traced BIOS path into a failure halt.
- Keyboard and auxiliary interface tests return `00h` for success.

## Validation

Smoke target:

```sh
cmake --build build/cmake-portable --target pc110kbdmcu-smoke
build/cmake-portable/pc110kbdmcu-smoke
```

The smoke test loads `Roms/M38813E4HP@QFP64.bin` and validates banner recognition, size, parsed version, self-test response, keyboard interface test response, keyboard/aux enable-disable state, command-byte writes, and counters.

Expected smoke output shape:

```text
keyboard MCU clean-room model ok: MELPS 740 Series Keyboard Firmware Version 1.1(C) Copyright 1992-1995 RIOS Systems Co.,Ltd. size=16255 version=1.1 checksum=8FC1D1C6
```

## Known Limits

- The model does not execute MELPS 740 instructions.
- Matrix scanning, debounce timing, Fn-key combinations, and exact interrupt timing are not modeled at the MCU level.
- The PS/2 auxiliary path is modeled only enough for current BIOS bring-up behavior.
- Unknown commands are recorded but otherwise treated as benign unless future traces show stronger behavior.
- The model intentionally avoids proprietary source reconstruction.
