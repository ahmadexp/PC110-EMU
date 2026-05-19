# PC110 EMU

Experimental macOS emulator for the IBM Palm Top PC 110.

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
