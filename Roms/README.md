Place legally obtained PC110 ROM dumps here:

- `pc110_bios.bin`: main PC110 BIOS image.
- `MSM538032E@SOP44.BIN`: optional Japanese font flash dump used for DBCS glyphs.
- `M38223E4HP@QFP80.BIN`: optional M3822x power-sense microcontroller firmware.
- `M38813E4HP@QFP64.bin`: optional MELPS 740 keyboard-controller firmware.

The emulator also accepts `PC110_FONT_ROM`, `PC110_MCU_FIRMWARE`, and
`PC110_KEYBOARD_MCU_FIRMWARE` environment variables when you want to keep those
dumps somewhere else.
