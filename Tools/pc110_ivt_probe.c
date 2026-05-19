#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "PC110Core/PC110Core.h"

static uint32_t read_vec21(PC110Machine *m) {
    uint32_t off = (uint32_t)pc110_mem_read8(m, 0x84u) |
                   ((uint32_t)pc110_mem_read8(m, 0x85u) << 8);
    uint32_t seg = (uint32_t)pc110_mem_read8(m, 0x86u) |
                   ((uint32_t)pc110_mem_read8(m, 0x87u) << 8);
    return (seg << 16) | off;
}

static uint32_t read_vec(PC110Machine *m, uint8_t intno) {
    uint32_t addr = (uint32_t)intno * 4u;
    uint32_t off = (uint32_t)pc110_mem_read8(m, addr) |
                   ((uint32_t)pc110_mem_read8(m, addr + 1u) << 8);
    uint32_t seg = (uint32_t)pc110_mem_read8(m, addr + 2u) |
                   ((uint32_t)pc110_mem_read8(m, addr + 3u) << 8);
    return (seg << 16) | off;
}

static uint32_t read_ptr(PC110Machine *m, uint32_t addr) {
    uint32_t off = (uint32_t)pc110_mem_read8(m, addr) |
                   ((uint32_t)pc110_mem_read8(m, addr + 1u) << 8);
    uint32_t seg = (uint32_t)pc110_mem_read8(m, addr + 2u) |
                   ((uint32_t)pc110_mem_read8(m, addr + 3u) << 8);
    return (seg << 16) | off;
}

int main(int argc, char **argv) {
    int max_steps = 2000000;
    int trace_from = -1;
    if (argc > 1) max_steps = atoi(argv[1]);
    if (argc > 2) trace_from = atoi(argv[2]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    pc110_load_bios(m, "Roms/pc110_bios.bin");
    pc110_attach_boot_image(m, "Disks/Disk1.img");

    uint32_t last = read_vec21(m);
    uint32_t last_req_buf = read_ptr(m, 0x1448u);
    printf("step=0 INT21=%04X:%04X\n", (unsigned)(last >> 16), (unsigned)(last & 0xFFFFu));
    printf("step=0 REQBUF=%04X:%04X\n",
           (unsigned)(last_req_buf >> 16), (unsigned)(last_req_buf & 0xFFFFu));

    for (int i = 0; i < max_steps; i++) {
        uint32_t pc = pc110_cpu_linear_pc(m);
        if (trace_from >= 0 && i >= trace_from) {
            pc110_trace_clear(m);
            pc110_cpu_set_trace_mode(m, 1);
        }

        pc110_cpu_step(m, 1);

        uint32_t now = read_vec21(m);
        if (now != last) {
            printf("step=%d pc_before=%08X INT21 %04X:%04X -> %04X:%04X\n",
                   i, (unsigned)pc,
                   (unsigned)(last >> 16), (unsigned)(last & 0xFFFFu),
                   (unsigned)(now >> 16), (unsigned)(now & 0xFFFFu));
            if (trace_from >= 0 && i >= trace_from) {
                char trace[8192];
                pc110_trace_copy(m, trace, sizeof(trace));
                puts(trace);
            }
            last = now;
        }
        uint32_t req_buf = read_ptr(m, 0x1448u);
        if (req_buf != last_req_buf) {
            printf("step=%d pc_before=%08X REQBUF %04X:%04X -> %04X:%04X\n",
                   i, (unsigned)pc,
                   (unsigned)(last_req_buf >> 16), (unsigned)(last_req_buf & 0xFFFFu),
                   (unsigned)(req_buf >> 16), (unsigned)(req_buf & 0xFFFFu));
            if (trace_from >= 0 && i >= trace_from) {
                char trace[8192];
                pc110_trace_copy(m, trace, sizeof(trace));
                puts(trace);
            }
            last_req_buf = req_buf;
        }
    }

    char state[12000];
    pc110_cpu_format_state(m, state, sizeof(state));
    uint8_t vectors[] = {0x08, 0x1C, 0x21, 0x28, 0x2A};
    for (unsigned i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint32_t v = read_vec(m, vectors[i]);
        printf("INT%02X=%04X:%04X\n", vectors[i], (unsigned)(v >> 16), (unsigned)(v & 0xFFFFu));
    }
    puts(state);
    pc110_destroy(m);
    return 0;
}
