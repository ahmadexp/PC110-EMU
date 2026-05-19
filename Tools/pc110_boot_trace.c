#include <stdio.h>
#include <stdlib.h>

#include "PC110Core/PC110Core.h"

static int attach_first_boot_image(PC110Machine *m) {
    if (pc110_attach_boot_image(m, "Disks/Personaware.PQI")) return 1;
    if (pc110_attach_boot_image(m, "Disks/Disk1.PQI")) return 1;
    if (pc110_attach_boot_image(m, "Disks/disk1.pqi")) return 1;
    if (pc110_attach_boot_image(m, "Disks/disk1.qpi")) return 1;
    if (pc110_attach_boot_image(m, "Disks/Disk1.qpi")) return 1;
    if (pc110_attach_boot_image(m, "Disks/Disk1.QPI")) return 1;
    if (pc110_attach_boot_image(m, "Disks/Disk1.img")) return 1;
    if (pc110_attach_boot_image(m, "Disks/disk.img")) return 1;
    return 0;
}

int main(int argc, char **argv) {
    int warmup = 30000000;
    int traced = 800;
    if (argc > 1) warmup = atoi(argv[1]);
    if (argc > 2) traced = atoi(argv[2]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    int bios = pc110_load_bios(m, "Roms/pc110_bios.bin");
    int img = attach_first_boot_image(m);

    pc110_cpu_set_trace_mode(m, 0);
    if (argc > 3) {
        uint32_t watch_start = (uint32_t)strtoul(argv[3], NULL, 0);
        uint32_t watch_end = argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 0) : watch_start;
        int min_step = argc > 5 ? atoi(argv[5]) : 0;
        int limit = warmup > 0 ? warmup : 5000000;
        for (int i = 0; i < limit; i++) {
            uint32_t pc = pc110_cpu_linear_pc(m);
            if (i >= min_step && pc >= watch_start && pc <= watch_end) {
                printf("watch hit step=%d pc=%08X\n", i, pc);
                break;
            }
            pc110_cpu_step(m, 1);
        }
    } else {
        pc110_cpu_step(m, warmup);
    }
    pc110_trace_clear(m);
    pc110_cpu_set_trace_mode(m, 1);
    pc110_cpu_step(m, traced);

    char state[12000];
    pc110_cpu_format_state(m, state, sizeof(state));
    printf("attach: bios=%d img=%d warmup=%d traced=%d\n", bios, img, warmup, traced);
    puts(state);

    size_t trace_size = 8u * 1024u * 1024u;
    char *trace = (char *)malloc(trace_size);
    if (trace) {
        pc110_trace_copy(m, trace, trace_size);
        puts(trace);
        free(trace);
    }

    pc110_destroy(m);
    return bios ? 0 : 1;
}
