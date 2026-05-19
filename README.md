# PC110 EMU

Experimental macOS emulator for the IBM Palm Top PC 110.

## Requirements

- macOS 13 or newer.
- Xcode Command Line Tools with Swift 5.9 or newer.
- A legally obtained IBM PC110 BIOS image.
- Optional boot media, such as a DOS or Personaware disk image.

Install the command line tools if `swift` is not available:

```sh
xcode-select --install
```

## How to Set Up Boot Assets

The app currently loads ROM and disk files from paths relative to the repository root. Run the emulator from this directory so those paths resolve correctly.

1. Put the PC110 BIOS at:

   ```text
   Roms/pc110_bios.bin
   ```

2. Put boot media in one of the supported locations. The emulator checks these paths:

   ```text
   Disks/img.ZIP
   Disks/Personaware.PQI
   Disks/Disk1.PQI
   Disks/disk1.pqi
   Disks/disk1.qpi
   Disks/Disk1.qpi
   Disks/Disk1.QPI
   ~/Desktop/Personaware.PQI
   Disks/Disk1.img
   Disks/disk.img
   ```

   For Personaware, prefer `Disks/disk1.qpi` or `Disks/Disk1.qpi`. The QPI/PQI names are checked before the legacy `Disks/Disk1.img`, so a correctly named Personaware image wins even if an old IMG is still present.

3. If you replace the BIOS or disk while the app is open, press `Reset` so the emulator reloads the assets.

## How to Build

From the repository root:

```sh
swift build
```

For a clean build:

```sh
swift package clean
swift build
```

## How to Run

From the repository root:

```sh
swift run PC110EMU
```

The window should report whether the BIOS and boot disk were loaded. A healthy startup with the included boot path usually reports a loaded BIOS and a boot disk name.

## How to Boot DOS

1. Start the app with `swift run PC110EMU`.
2. Confirm the status line says the BIOS is loaded.
3. Press `Continue Run`.
4. Wait for the display or text diagnostics to reach the DOS startup path.
5. Press `Pause Run` when you want to inspect the machine state.

This milestone is a stable DOS-visible recovery point, not a claim of full DOS completion. The emulator may still be inside the `IO.SYS` loader path.

## How to Boot Personaware

1. Put your Personaware image at one of these preferred paths:

   ```text
   Disks/disk1.qpi
   Disks/Disk1.qpi
   Disks/Personaware.PQI
   ```

2. Start the emulator:

   ```sh
   swift run PC110EMU
   ```

3. Confirm the status line shows the BIOS is loaded and the boot disk is your QPI/PQI image.
4. Press `Continue Run`.
5. When DOS reaches a prompt, run:

   ```text
   cd pw
   pw
   ```

The emulator repairs Personaware boot scripts in memory when it detects a PC DOS image with a `PW` directory. This avoids loading `HIMEM.SYS` on the incomplete A20 path and exposes the emulator XMS shim to Personaware/DOSPM. The disk image on disk is not modified.

## How to Use Input

- Click the display area or main window before typing.
- Printable ASCII keys are sent to the emulated machine.
- Control-key text input such as `Control-C` is forwarded when applicable.
- Command and Option shortcuts remain reserved for macOS.
- Mouse movement and button events are forwarded when the pointer is inside the emulated display.

## How to Use Diagnostics

The right pane contains copy-ready diagnostic views:

- `CPU` shows registers and current execution state.
- `Trace` shows the accumulated trace log.
- `Memory` reads memory from the address in the address field.
- `Text` shows the current text-mode screen contents.

Useful buttons:

- `Copy Bundle` copies CPU state, trace tail, and text screen.
- `Copy Text` copies the text-mode screen dump.
- `Clear Trace` clears the accumulated trace log.
- `F1 Setup` induces the BIOS F1 setup path and copies a status bundle.
- `Easy Setup` enters the ROM-backed setup path and copies a status bundle.

## How to Check the C Core

The Swift package build compiles the C core automatically. To compile only the core during bring-up work:

```sh
clang -std=c99 -Wall -I Sources/PC110Core/include -c Sources/PC110Core/pc110_core.c -o /tmp/pc110_core.o
```

To run the Personaware diagnostic capture:

```sh
clang -std=c99 -Wall -I Sources/PC110Core/include Tools/pc110_frame_dump.c Sources/PC110Core/pc110_core.c -o /tmp/pc110_frame_dump
/tmp/pc110_frame_dump /tmp/personaware.bmp boot 4700000 "cd pw,enter,pw,enter" 30000000
```

## Troubleshooting

- `No BIOS loaded`: confirm `Roms/pc110_bios.bin` exists and that you launched the app from the repository root.
- `Boot disk: none`: add a disk image using one of the supported filenames above, then press `Reset`.
- `Boot disk: Disk1.img` when you expected Personaware: confirm the file is named `Disks/disk1.qpi`, `Disks/Disk1.qpi`, or `Disks/Personaware.PQI`, then press `Reset`.
- Blank or stale display: press `Reset`, then `Continue Run`.
- Diagnostics do not update while paused: press the relevant refresh or copy button, or resume briefly with `Continue Run`.

## Milestone 16.78

This continues the 16.75 stable boot line while adding targeted protected-mode and Personaware boot support.

## What 16.78 adds

- INT 2F XMS install and entry-point responses.
- A small real-mode XMS entry stub for version, free memory, allocation, free, and move calls.
- INT 33 mouse-driver responses backed by the emulator pointer state.
- 80186 BOUND instruction handling with INT 5 dispatch and fault diagnostics.
- Extra SS/ES-prefixed instruction coverage used by DOS and Personaware paths.
- Additional PQI/QPI boot image filename candidates.
- Protected-mode FS/GS and `67 66` SIB addressing coverage used by Personaware DOSPM startup.
- In-memory Personaware CONFIG/AUTOEXEC repair to avoid the unsupported HIMEM/A20 boot path.
- QPI-first boot image probing in the Personaware frame dump and boot trace tools.

## Milestone 16.77

This continued the 16.75 stable boot line while adding narrow compatibility shims for Personaware and ROM setup exploration.

## Milestone 16.75

This is a stability recovery build.

## Why this build exists

The 16.68 through 16.74 line was useful for investigating real-ROM Easy Setup, but it regressed the normal boot experience:

```text
INT19: calls=0
Boot IMG: int19_loads=0
```

That means it never reached DOS.

16.75 returns to the 16.67 DOS-visible line as the base, then applies a narrow user-visible fix for the repeated DOS startup banner.

## What 16.75 does

Keeps the last DOS-reaching branch:

```text
INT19=1
Boot IMG int19_loads=1
Starting MS-DOS visible
```

Adds B800 de-duplication for the DOS banner:

```text
Starting MS-DOS...
```

The first banner row is kept. Later duplicate rows are cleared from B800 before framebuffer rendering and before diagnostic text-screen formatting.

New diagnostic:

```text
DOS banner de-dup: scans=N clears=N
```

## What this does not claim

This does not mean DOS is fully booted. The emulator can still be inside the IO.SYS loader path. This build is intentionally a stable user-facing recovery point while real-ROM Easy Setup stays separate.

## Recommended next work

Keep two branches:

```text
stable-boot
real-easysetup-experimental
```

Do not merge real-ROM Easy Setup changes into stable boot until they actually enter the ROM's graphical setup UI.
