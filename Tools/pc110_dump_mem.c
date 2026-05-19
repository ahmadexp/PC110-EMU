#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "PC110Core/PC110Core.h"

static int dump_range(PC110Machine *m, uint32_t start, uint32_t length, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "open %s failed: %d\n", path, errno);
        return 0;
    }
    for (uint32_t i = 0; i < length; i++) {
        uint8_t b = pc110_mem_read8(m, start + i);
        if (fwrite(&b, 1, 1, f) != 1) {
            fprintf(stderr, "write %s failed\n", path);
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    int steps = 30000000;
    if (argc > 1) steps = atoi(argv[1]);

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    if (!pc110_load_bios(m, "Roms/pc110_bios.bin")) {
        pc110_destroy(m);
        return 1;
    }
    (void)pc110_attach_boot_image(m, "Disks/Disk1.img");
    pc110_cpu_step(m, steps);

    int ok = 1;
    ok &= dump_range(m, 0x00000700u, 0xD000u, "/private/tmp/pc110_0070.bin");
    ok &= dump_range(m, 0x00002A70u, 0x2000u, "/private/tmp/pc110_02a7.bin");
    ok &= dump_range(m, 0x0001FF00u, 0x1000u, "/private/tmp/pc110_1ff0.bin");
    ok &= dump_range(m, 0x000A160u, 0xA000u, "/private/tmp/pc110_0a16.bin");
    ok &= dump_range(m, 0x0008E000u, 0x9000u, "/private/tmp/pc110_8e00.bin");
    ok &= dump_range(m, 0x0008E900u, 0x3000u, "/private/tmp/pc110_8e90.bin");
    ok &= dump_range(m, 0x00090530u, 0xD000u, "/private/tmp/pc110_9053.bin");
    ok &= dump_range(m, 0x0009E390u, 0x2000u, "/private/tmp/pc110_9e39.bin");
    ok &= dump_range(m, 0x0009FC00u, 0x2000u, "/private/tmp/pc110_9fc0.bin");

    char state[4096];
    pc110_cpu_format_state(m, state, sizeof(state));
    puts(state);
    pc110_destroy(m);
    return ok ? 0 : 1;
}
