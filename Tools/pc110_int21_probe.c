#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "PC110Core/PC110Core.h"

int main(int argc, char **argv) {
    int max_steps = 2000000;
    if (argc > 1) max_steps = atoi(argv[1]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    pc110_load_bios(m, "Roms/pc110_bios.bin");
    pc110_attach_boot_image(m, "Disks/Disk1.img");

    for (int i = 0; i < max_steps; i++) {
        uint32_t pc = pc110_cpu_linear_pc(m);
        if (pc110_mem_read8(m, pc) == 0xCDu && pc110_mem_read8(m, pc + 1u) == 0x21u) {
            pc110_trace_clear(m);
            pc110_cpu_set_trace_mode(m, 1);
            pc110_cpu_step(m, 1);
            pc110_cpu_set_trace_mode(m, 0);
            char trace[4096];
            pc110_trace_copy(m, trace, sizeof(trace));
            printf("step=%d pc=%08X\n%s", i, pc, trace);
        } else {
            pc110_cpu_step(m, 1);
        }
    }

    char state[12000];
    pc110_cpu_format_state(m, state, sizeof(state));
    puts(state);
    pc110_destroy(m);
    return 0;
}
