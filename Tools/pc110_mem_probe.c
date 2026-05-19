#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "PC110Core/PC110Core.h"

static uint16_t rd16(PC110Machine *m, uint32_t addr) {
    return (uint16_t)pc110_mem_read8(m, addr) |
           (uint16_t)((uint16_t)pc110_mem_read8(m, addr + 1u) << 8);
}

static void dump_word(PC110Machine *m, uint16_t seg, uint16_t off) {
    uint32_t phys = ((uint32_t)seg << 4) + off;
    printf("%04X:%04X phys=%05X %04X\n", seg, off, phys, rd16(m, phys));
}

static void dump_bytes(PC110Machine *m, uint16_t seg, uint16_t off, unsigned count) {
    uint32_t phys = ((uint32_t)seg << 4) + off;
    printf("%04X:%04X phys=%05X", seg, off, phys);
    for (unsigned i = 0; i < count; i++) {
        printf(" %02X", pc110_mem_read8(m, phys + i));
    }
    printf("  ");
    for (unsigned i = 0; i < count; i++) {
        unsigned ch = pc110_mem_read8(m, phys + i);
        putchar((ch >= 0x20u && ch <= 0x7Eu) ? (int)ch : '.');
    }
    putchar('\n');
}

int main(int argc, char **argv) {
    int steps = 1200000;
    if (argc > 1) steps = atoi(argv[1]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    pc110_load_bios(m, "Roms/pc110_bios.bin");
    pc110_attach_boot_image(m, "Disks/Disk1.img");
    pc110_cpu_set_trace_mode(m, 0);
    pc110_cpu_step(m, steps);

    printf("steps=%d pc=%08X\n", steps, pc110_cpu_linear_pc(m));
    dump_bytes(m, 0x007Bu, 0x8C30u, 128u);
    dump_bytes(m, 0x007Bu, 0x8CB0u, 128u);
    dump_bytes(m, 0x007Bu, 0x8D30u, 128u);
    dump_bytes(m, 0x007Bu, 0x5980u, 64u);
    dump_word(m, 0x0070u, 0x011Bu);
    dump_word(m, 0x0070u, 0x0123u);
    dump_word(m, 0x0070u, 0x0125u);
    dump_word(m, 0x0070u, 0x012Bu);
    dump_word(m, 0x0070u, 0x0119u);
    dump_bytes(m, 0x0070u, 0x0352u, 100u);
    dump_bytes(m, 0x0070u, 0x03B6u, 100u);
    dump_bytes(m, 0x0070u, 0x0350u, 128u);
    dump_bytes(m, 0x0070u, 0x03B0u, 128u);
    dump_bytes(m, 0x0070u, 0x0410u, 160u);
    dump_bytes(m, 0x0070u, 0x0790u, 192u);
    dump_bytes(m, 0x0000u, 0x0470u, 32u);
    dump_bytes(m, 0x0000u, 0x0500u, 256u);
    dump_bytes(m, 0x7000u, 0x0000u, 128u);
    dump_bytes(m, 0x8000u, 0x0000u, 128u);
    dump_bytes(m, 0x9000u, 0x0000u, 128u);
    dump_bytes(m, 0x02A7u, 0x05A0u, 64u);
    dump_bytes(m, 0x02A7u, 0x0A00u, 128u);
    dump_word(m, 0x0070u, 0x16D6u);
    dump_word(m, 0x0070u, 0x16D8u);
    dump_word(m, 0x0070u, 0x16DAu);
    dump_word(m, 0x0070u, 0x16DCu);
    dump_word(m, 0x0070u, 0x16DEu);
    dump_word(m, 0x0070u, 0x16DFu);
    dump_word(m, 0x0070u, 0x16E4u);
    dump_word(m, 0x0070u, 0x16E8u);
    dump_word(m, 0x9FC0u, 0x0000u);
    dump_word(m, 0x9FC0u, 0x10F8u);
    dump_bytes(m, 0x907Au, 0x0260u, 64u);
    dump_bytes(m, 0x907Au, 0x02A0u, 64u);
    dump_bytes(m, 0x907Au, 0x0370u, 64u);
    dump_bytes(m, 0x907Au, 0x3F70u, 64u);
    dump_bytes(m, 0x907Au, 0x22C0u, 64u);
    dump_bytes(m, 0x907Au, 0x22E0u, 64u);
    dump_bytes(m, 0x907Au, 0x55C0u, 128u);
    dump_bytes(m, 0x907Au, 0x8F00u, 288u);
    dump_bytes(m, 0x907Au, 0xCE30u, 96u);
    dump_bytes(m, 0x907Au, 0xFB00u, 128u);
    dump_bytes(m, 0x8EC1u, 0x0520u, 64u);
    dump_bytes(m, 0x907Au, 0xBA60u, 80u);
    dump_bytes(m, 0x907Au, 0xBB30u, 352u);
    dump_bytes(m, 0x907Au, 0xC2E0u, 128u);
    dump_bytes(m, 0x907Au, 0xC3E0u, 96u);
    dump_bytes(m, 0x907Au, 0x8E80u, 128u);
    dump_bytes(m, 0x753Du, 0x3C00u, 64u);
    dump_bytes(m, 0x1FF0u, 0x00E0u, 64u);
    dump_bytes(m, 0x02CDu, 0x55C0u, 128u);
    dump_bytes(m, 0x02CDu, 0x0000u, 64u);
    dump_bytes(m, 0x02CDu, 0x19B0u, 128u);
    dump_bytes(m, 0x02CDu, 0x1C20u, 128u);
    dump_bytes(m, 0x02CDu, 0xD5F0u, 96u);
    dump_bytes(m, 0x02CDu, 0xD640u, 128u);
    dump_bytes(m, 0x02CDu, 0xEA00u, 128u);
    dump_bytes(m, 0x02CDu, 0xEB10u, 128u);
    dump_bytes(m, 0x02CDu, 0xFB00u, 128u);
    dump_bytes(m, 0x0000u, 0x0080u, 96u);
    dump_bytes(m, 0x0000u, 0x0090u, 64u);
    dump_bytes(m, 0x8EC1u, 0x04F0u, 48u);
    dump_bytes(m, 0x01EBu, 0x00B0u, 160u);
    dump_bytes(m, 0x01EBu, 0x03A0u, 160u);
    dump_bytes(m, 0x0EFEu, 0x08C0u, 160u);
    dump_bytes(m, 0x0EFEu, 0xA840u, 96u);
    dump_bytes(m, 0x8F71u, 0x72E0u, 96u);
    dump_bytes(m, 0xB8FFu, 0xFFB0u, 64u);
    dump_bytes(m, 0xB8FFu, 0x55C0u, 64u);

    char text[4096];
    pc110_debug_format_text_screen(m, text, sizeof(text));
    puts(text);

    pc110_destroy(m);
    return 0;
}
