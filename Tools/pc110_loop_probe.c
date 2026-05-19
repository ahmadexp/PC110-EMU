#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PC110Core/PC110Core.h"

static uint16_t rd16(PC110Machine *m, uint32_t addr) {
    return (uint16_t)pc110_mem_read8(m, addr) |
           (uint16_t)((uint16_t)pc110_mem_read8(m, addr + 1u) << 8);
}

static uint32_t phys(unsigned seg, unsigned off) {
    return ((uint32_t)(seg & 0xFFFFu) << 4) + (uint32_t)(off & 0xFFFFu);
}

static int parse_segments(const char *state, unsigned *ds, unsigned *es, unsigned *ss) {
    const char *p = strstr(state, "DS ES SS FS GS:");
    if (!p) return 0;
    return sscanf(p, "DS ES SS FS GS:  %x %x %x", ds, es, ss) == 3;
}

static int parse_gp_regs(const char *state, unsigned *ax, unsigned *bx, unsigned *cx, unsigned *dx) {
    const char *p = strstr(state, "EAX EBX ECX EDX:");
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (!p) return 0;
    if (sscanf(p, "EAX EBX ECX EDX: %x %x %x %x", &eax, &ebx, &ecx, &edx) != 4) return 0;
    if (ax) *ax = eax & 0xFFFFu;
    if (bx) *bx = ebx & 0xFFFFu;
    if (cx) *cx = ecx & 0xFFFFu;
    if (dx) *dx = edx & 0xFFFFu;
    return 1;
}

static int parse_pc_regs(const char *state, unsigned *cs, unsigned *ip, unsigned *flags, int *cf) {
    const char *p = strstr(state, "CS:IP:");
    unsigned parsed_cs = 0, parsed_ip = 0;
    if (!p || sscanf(p, "CS:IP:    %x:%x", &parsed_cs, &parsed_ip) != 2) return 0;
    if (cs) *cs = parsed_cs & 0xFFFFu;
    if (ip) *ip = parsed_ip & 0xFFFFu;

    p = strstr(state, "EFLAGS:");
    unsigned parsed_flags = 0;
    if (p && sscanf(p, "EFLAGS:   %x", &parsed_flags) == 1) {
        if (flags) *flags = parsed_flags;
        if (cf) *cf = (parsed_flags & 1u) != 0u;
    }
    return 1;
}

static void read_cstr(PC110Machine *m, unsigned seg, unsigned off, char *out, size_t out_size) {
    if (!out || out_size == 0) return;
    size_t pos = 0;
    uint32_t base = phys(seg, 0);
    while (pos + 1 < out_size) {
        uint8_t ch = pc110_mem_read8(m, base + ((off + (unsigned)pos) & 0xFFFFu));
        if (ch == 0) break;
        out[pos++] = (ch >= 0x20u && ch <= 0x7Eu) ? (char)ch : '.';
    }
    out[pos] = 0;
}

static void dump_bytes(PC110Machine *m, uint32_t addr, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        if ((i & 15u) == 0u) printf("\n  %05X:", addr + i);
        printf(" %02X", pc110_mem_read8(m, addr + i));
    }
    printf("\n");
}

static void print_dpb_chain(PC110Machine *m, unsigned ds) {
    uint32_t list_phys = phys(ds, 0x0119u);
    uint16_t off = rd16(m, list_phys);
    uint16_t seg = rd16(m, list_phys + 2u);

    printf("DPB list DS:0119 -> %04X:%04X\n", seg, off);
    for (unsigned i = 0; i < 12u; i++) {
        if (off == 0x0000u || off == 0xFFFFu || seg == 0x0000u) break;
        uint32_t p = phys(seg, off);
        uint16_t next_off = rd16(m, p);
        uint16_t next_seg = rd16(m, p + 2u);
        char fat_type[9];
        for (unsigned j = 0; j < 8u; j++) {
            uint8_t ch = pc110_mem_read8(m, p + 0x5Bu + j);
            fat_type[j] = (ch >= 0x20u && ch <= 0x7Eu) ? (char)ch : '.';
        }
        fat_type[8] = 0;

        printf("  dpb[%u] %04X:%04X next=%04X:%04X unit=%02X logical=%02X bps=%u spc=%u media=%02X fats=%u root=%u first_data=%u max_cluster=%u fat16flag=%02X fat_type=%s\n",
               i, seg, off, next_seg, next_off,
               pc110_mem_read8(m, p + 4u),
               pc110_mem_read8(m, p + 5u),
               rd16(m, p + 6u),
               pc110_mem_read8(m, p + 8u),
               pc110_mem_read8(m, p + 0x10u),
               pc110_mem_read8(m, p + 0x16u),
               rd16(m, p + 0x11u),
               rd16(m, p + 0x19u),
               rd16(m, p + 0x1Du),
               pc110_mem_read8(m, p + 0x3Bu),
               fat_type);
        printf("    raw:");
        dump_bytes(m, p, 0x70u);
        if (next_off == 0xFFFFu) break;
        if (next_off == off && next_seg == seg) break;
        off = next_off;
        seg = next_seg;
    }
}

static void print_text(PC110Machine *m) {
    char text[8192];
    pc110_debug_format_text_screen(m, text, sizeof(text));
    puts("TEXT SCREEN:");
    puts(text);
}

static void print_final_probe(PC110Machine *m, int step) {
    char state[12000];
    unsigned ds = 0, es = 0, ss = 0;
    pc110_cpu_format_state(m, state, sizeof(state));
    parse_segments(state, &ds, &es, &ss);
    puts(state);

    uint32_t ss_base = phys(ss, 0);
    uint32_t ds_base = phys(ds, 0);
    printf("final step=%d pc=%08X DS=%04X ES=%04X SS=%04X\n",
           step, pc110_cpu_linear_pc(m), ds, es, ss);

    printf("SS vars: 0357=%02X 0358=%02X 035A=%02X 035B=%02X 0378=%04X 037A=%04X 0AA0=%02X 0D07=%02X 0D08=%02X 0D0C=%02X 0D90=%02X 0D91=%02X 0EBB=%04X\n",
           pc110_mem_read8(m, ss_base + 0x0357u),
           pc110_mem_read8(m, ss_base + 0x0358u),
           pc110_mem_read8(m, ss_base + 0x035Au),
           pc110_mem_read8(m, ss_base + 0x035Bu),
           rd16(m, ss_base + 0x0378u),
           rd16(m, ss_base + 0x037Au),
           pc110_mem_read8(m, ss_base + 0x0AA0u),
           pc110_mem_read8(m, ss_base + 0x0D07u),
           pc110_mem_read8(m, ss_base + 0x0D08u),
           pc110_mem_read8(m, ss_base + 0x0D0Cu),
           pc110_mem_read8(m, ss_base + 0x0D90u),
           pc110_mem_read8(m, ss_base + 0x0D91u),
           rd16(m, ss_base + 0x0EBBu));

    printf("DS pointers: [0000]=%04X [0006]=%04X [0119]=%04X:%04X [0346]=%04X [037F]=%04X [0380]=%04X [0389]=%04X [03BC]=%04X\n",
           rd16(m, ds_base + 0x0000u),
           rd16(m, ds_base + 0x0006u),
           rd16(m, ds_base + 0x011Bu),
           rd16(m, ds_base + 0x0119u),
           rd16(m, ds_base + 0x0346u),
           rd16(m, ds_base + 0x037Fu),
           rd16(m, ds_base + 0x0380u),
           rd16(m, ds_base + 0x0389u),
           rd16(m, ds_base + 0x03BCu));

    puts("SS:00A0 low pointer table:");
    dump_bytes(m, ss_base + 0x00A0u, 0x40u);
    puts("CS:44F0 callback target bytes:");
    dump_bytes(m, phys(0x0029u, 0x44F0u), 0x40u);
    puts("0070:0500 device trampoline:");
    dump_bytes(m, phys(0x0070u, 0x0500u), 0x140u);
    puts("SS:0350 request area:");
    dump_bytes(m, ss_base + 0x0350u, 0x60u);
    puts("SS:0D00 vars:");
    dump_bytes(m, ss_base + 0x0D00u, 0xB0u);
    print_dpb_chain(m, ds);
    print_text(m);
}

static void print_ptrs(PC110Machine *m, int step, uint32_t pc) {
    char state[12000];
    unsigned ds = 0, es = 0, ss = 0;
    pc110_cpu_format_state(m, state, sizeof(state));
    if (!parse_segments(state, &ds, &es, &ss)) {
        printf("step=%d pc=%08X parse failed\n", step, pc);
        return;
    }

    uint32_t ss_base = (uint32_t)ss << 4;
    uint16_t p001e_off = rd16(m, ss_base + 0x001eu);
    uint16_t p001e_seg = rd16(m, ss_base + 0x0020u);
    uint16_t p006d_off = rd16(m, ss_base + 0x006du);
    uint16_t p006d_seg = rd16(m, ss_base + 0x006fu);
    uint16_t sent = rd16(m, ss_base + 0x0ebbu);
    uint32_t ds_base = (uint32_t)ds << 4;

    printf("step=%d pc=%08X DS=%04X ES=%04X SS=%04X ss:001e=%04X:%04X ss:006d=%04X:%04X ss:0ebb=%04X ds[0]=%04X ds[6]=%04X ds[414a]=%04X ds[4150]=%04X\n",
           step, pc, ds, es, ss,
           p001e_seg, p001e_off, p006d_seg, p006d_off, sent,
           rd16(m, ds_base + 0u), rd16(m, ds_base + 6u),
           rd16(m, ds_base + 0x414au), rd16(m, ds_base + 0x4150u));
}

int main(int argc, char **argv) {
    int limit = 30000000;
    if (argc > 1) limit = atoi(argv[1]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    pc110_load_bios(m, "Roms/pc110_bios.bin");
    pc110_attach_boot_image(m, "Disks/Disk1.img");
    pc110_cpu_set_trace_mode(m, 0);

    int final_only = argc > 2 && strcmp(argv[2], "final") == 0;
    int int13_only = argc > 2 && strcmp(argv[2], "int13") == 0;
    int int16_only = argc > 2 && strcmp(argv[2], "int16") == 0;
    int int2f_only = argc > 2 && strcmp(argv[2], "int2f") == 0;
    int int21_only = argc > 2 && strcmp(argv[2], "int21") == 0;
    int int21_return_only = argc > 2 && strcmp(argv[2], "int21ret") == 0;
    int poke_0358 = argc > 2 && strcmp(argv[2], "poke0358") == 0;
    int poke_0aa0 = argc > 2 && strcmp(argv[2], "poke0aa0") == 0;
    int poke_0d0c = argc > 2 && strcmp(argv[2], "poke0d0c") == 0;
    if (poke_0358 || poke_0aa0 || poke_0d0c) {
        int before = argc > 3 ? atoi(argv[3]) : limit;
        int after = argc > 4 ? atoi(argv[4]) : 5000000;
        pc110_cpu_step(m, before);
        char state[12000];
        unsigned ds = 0, es = 0, ss = 0;
        pc110_cpu_format_state(m, state, sizeof(state));
        parse_segments(state, &ds, &es, &ss);
        uint32_t ss_base = phys(ss, 0);
        if (poke_0358) pc110_mem_write8(m, ss_base + 0x0358u, 1u);
        if (poke_0aa0) pc110_mem_write8(m, ss_base + 0x0AA0u, 1u);
        if (poke_0d0c) pc110_mem_write8(m, ss_base + 0x0D0Cu, 1u);
        printf("poked at step=%d mode=%s SS=%04X now continuing %d\n",
               before, argv[2], ss, after);
        pc110_cpu_step(m, after);
        print_final_probe(m, before + after);
        pc110_destroy(m);
        return 0;
    }

    int hits = 0;
    for (int i = 0; i < limit; i++) {
        if (int13_only || int16_only || int2f_only || int21_only || int21_return_only) {
            uint32_t pc = pc110_cpu_linear_pc(m);
            uint8_t wanted_int = int13_only ? 0x13u : (int16_only ? 0x16u : (int2f_only ? 0x2Fu : 0x21u));
            if (pc110_mem_read8(m, pc) == 0xCDu && pc110_mem_read8(m, pc + 1u) == wanted_int) {
                char state[12000];
                char trace[4096];
                unsigned ax = 0, bx = 0, cx = 0, dx = 0;
                unsigned ds = 0, es = 0, ss = 0;
                pc110_cpu_format_state(m, state, sizeof(state));
                parse_gp_regs(state, &ax, &bx, &cx, &dx);
                parse_segments(state, &ds, &es, &ss);
                char path[160];
                path[0] = 0;
                if (!int13_only && ((ax >> 8) == 0x3Du || (ax >> 8) == 0x4Bu)) {
                    read_cstr(m, ds, dx, path, sizeof(path));
                }
                if (!int13_only && (ax >> 8) == 0x4Bu) {
                    uint32_t block = phys(es, bx);
                    uint16_t env = rd16(m, block);
                    uint16_t tail_off = rd16(m, block + 2u);
                    uint16_t tail_seg = rd16(m, block + 4u);
                    uint32_t tail = phys(tail_seg, tail_off);
                    uint8_t tail_len = pc110_mem_read8(m, tail);
                    printf("exec param ES:BX=%04X:%04X env=%04X tail=%04X:%04X len=%u text=",
                           es, bx, env, tail_seg, tail_off, (unsigned)tail_len);
                    for (unsigned n = 0; n < tail_len && n < 96u; n++) {
                        uint8_t ch = pc110_mem_read8(m, tail + 1u + n);
                        putchar((ch >= 0x20u && ch <= 0x7Eu) ? (char)ch : '.');
                    }
                    putchar('\n');
                }
                if (int16_only) {
                    unsigned cs = 0, ip = 0, flags = 0;
                    int cf = 0;
                    parse_pc_regs(state, &cs, &ip, &flags, &cf);
                    printf("int16 call step=%d pc=%08X CS:IP=%04X:%04X AX=%04X DS=%04X ES=%04X SS=%04X\n",
                           i, pc, cs, ip, ax, ds, es, ss);
                }
                if (int2f_only) {
                    unsigned cs = 0, ip = 0, flags = 0;
                    int cf = 0;
                    parse_pc_regs(state, &cs, &ip, &flags, &cf);
                    uint16_t vec_ip = (uint16_t)(pc110_mem_read8(m, 0x2Fu * 4u) |
                                                 ((uint16_t)pc110_mem_read8(m, 0x2Fu * 4u + 1u) << 8));
                    uint16_t vec_cs = (uint16_t)(pc110_mem_read8(m, 0x2Fu * 4u + 2u) |
                                                 ((uint16_t)pc110_mem_read8(m, 0x2Fu * 4u + 3u) << 8));
                    printf("int2f call step=%d pc=%08X CS:IP=%04X:%04X AX=%04X BX=%04X CX=%04X DX=%04X DS=%04X ES=%04X SS=%04X vec=%04X:%04X\n",
                           i, pc, cs, ip, ax, bx, cx, dx, ds, es, ss, vec_cs, vec_ip);
                }
                if (int21_return_only && !int13_only) {
                    unsigned cs = 0, ip = 0, flags = 0;
                    int cf = 0;
                    parse_pc_regs(state, &cs, &ip, &flags, &cf);
                    uint32_t ret_pc = phys(cs, (ip + 2u) & 0xFFFFu);
                    unsigned ah = (ax >> 8) & 0xFFu;
                    int interesting = (ah == 0x3Du || ah == 0x3Fu || ah == 0x42u || ah == 0x4Bu);
                    if (interesting) {
                        printf("call step=%d pc=%08X AH=%02X AX=%04X BX=%04X CX=%04X DX=%04X DS=%04X ES=%04X SS=%04X path=%s ret=%08X\n",
                               i, pc, ah, ax, bx, cx, dx, ds, es, ss, path, ret_pc);
                    }
                    pc110_cpu_step(m, 1);
                    int waited = 1;
                    while (waited < 800000 && pc110_cpu_linear_pc(m) != ret_pc) {
                        pc110_cpu_step(m, 1);
                        waited++;
                    }
                    i += waited;
                    pc110_cpu_format_state(m, state, sizeof(state));
                    parse_gp_regs(state, &ax, &bx, &cx, &dx);
                    parse_pc_regs(state, &cs, &ip, &flags, &cf);
                    if (interesting) {
                        printf("ret  step=%d waited=%d pc=%08X CF=%d AX=%04X BX=%04X CX=%04X DX=%04X FLAGS=%04X\n",
                               i, waited, pc110_cpu_linear_pc(m), cf, ax, bx, cx, dx, flags & 0xFFFFu);
                    }
                    continue;
                }
                pc110_trace_clear(m);
                pc110_cpu_set_trace_mode(m, 1);
                pc110_cpu_step(m, 1);
                pc110_cpu_set_trace_mode(m, 0);
                pc110_trace_copy(m, trace, sizeof(trace));
                printf("step=%d pc=%08X AX=%04X BX=%04X CX=%04X DX=%04X DS=%04X ES=%04X SS=%04X path=%s %s",
                       i, pc, ax, bx, cx, dx, ds, es, ss, path, trace);
                continue;
            }
            pc110_cpu_step(m, 1);
            continue;
        }
        if (final_only) {
            pc110_cpu_step(m, 1);
            continue;
        }
        uint32_t pc = pc110_cpu_linear_pc(m);
        if (pc == 0x0009A024u || pc == 0x0009A035u || pc == 0x0009A047u) {
            char state[12000];
            unsigned ds = 0, es = 0, ss = 0;
            pc110_cpu_format_state(m, state, sizeof(state));
            parse_segments(state, &ds, &es, &ss);
            uint16_t sent = rd16(m, ((uint32_t)ss << 4) + 0x0ebbu);
            if (hits < 20 || sent == 0x414au) {
                print_ptrs(m, i, pc);
            }
            hits++;
            if (sent == 0x414au) {
                pc110_trace_clear(m);
                pc110_cpu_set_trace_mode(m, 1);
                pc110_cpu_step(m, 220);
                pc110_cpu_format_state(m, state, sizeof(state));
                puts(state);
                char trace[32000];
                pc110_trace_copy(m, trace, sizeof(trace));
                puts(trace);
                pc110_destroy(m);
                return 0;
            }
        }
        pc110_cpu_step(m, 1);
    }

    if (final_only || int13_only || int21_only || int21_return_only) {
        print_final_probe(m, limit);
        pc110_destroy(m);
        return 0;
    }

    printf("limit reached hits=%d pc=%08X\n", hits, pc110_cpu_linear_pc(m));
    pc110_destroy(m);
    return 0;
}
