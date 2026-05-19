#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PC110Core/PC110Core.h"

static int attach_first_boot_image(PC110Machine *m) {
    if (pc110_attach_boot_image(m, "Disks/Disk1.img")) return 1;
    if (pc110_attach_boot_image(m, "Disks/disk.img")) return 1;
    return 0;
}

static int line_contains(const char *line, size_t len, const char *needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0 || nlen > len) return 0;
    for (size_t i = 0; i + nlen <= len; i++) {
        if (memcmp(line + i, needle, nlen) == 0) return 1;
    }
    return 0;
}

static void print_filtered_trace(PC110Machine *m) {
    char *trace = (char *)malloc(1200000u);
    if (!trace) return;
    pc110_trace_copy(m, trace, 1200000u);

    const char *line = trace;
    while (*line) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        if ((len && line_contains(line, len, "Boot IMG")) ||
            (len && line_contains(line, len, "CD 13")) ||
            (len && line_contains(line, len, "INT13")) ||
            (len && line_contains(line, len, "block read request")) ||
            (len && line_contains(line, len, "CONFIG")) ||
            (len && line_contains(line, len, "STACK")) ||
            (len && line_contains(line, len, "KBC data")) ||
            (len && line_contains(line, len, "KBC status")) ||
            (len && line_contains(line, len, "F1")) ||
            (len && line_contains(line, len, "000F885")) ||
            (len && line_contains(line, len, "000F666"))) {
            fwrite(line, 1, len, stdout);
            fputc('\n', stdout);
        }
        if (!end) break;
        line = end + 1;
    }

    free(trace);
}

int main(int argc, char **argv) {
    int steps = 30000000;
    int trace = 1;
    const char *mode = "";
    int filter_trace = 0;

    if (argc > 1) {
        steps = atoi(argv[1]);
        if (steps <= 0) steps = 30000000;
    }
    if (argc > 2) {
        trace = atoi(argv[2]) != 0;
    }
    if (argc > 3) {
        mode = argv[3];
    }
    if (argc > 4) {
        filter_trace = strcmp(argv[4], "filter") == 0;
    }

    PC110Machine *m = pc110_create();
    if (!m) {
        fputs("create failed\n", stderr);
        return 2;
    }

    int bios = pc110_load_bios(m, "Roms/pc110_bios.bin");
    int img = attach_first_boot_image(m);
    int zip = pc110_attach_boot_zip(m, "Disks/img.ZIP");

    pc110_cpu_set_trace_mode(m, trace);
    if (strcmp(mode, "setup") == 0) {
        pc110_enter_easy_setup(m);
    } else if (strcmp(mode, "f1") == 0) {
        pc110_induce_f1(m);
    }
    pc110_cpu_step(m, steps);

    char state[12000];
    pc110_cpu_format_state(m, state, sizeof(state));
    printf("attach: bios=%d zip=%d img=%d steps=%d trace=%d mode=%s\n",
           bios, zip, img, steps, trace, mode[0] ? mode : "boot");
    puts(state);

    if (filter_trace) {
        print_filtered_trace(m);
    } else {
        char tail[24000];
        pc110_trace_copy(m, tail, sizeof(tail));
        size_t n = 0;
        while (tail[n]) n++;
        size_t start = n > 8000 ? n - 8000 : 0;
        puts(tail + start);
    }

    pc110_destroy(m);
    return bios ? 0 : 1;
}
