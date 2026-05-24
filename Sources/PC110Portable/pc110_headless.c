#include "pc110_portable_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HeadlessOptions {
    PC110PortableConfig config;
    const char *output_bmp;
    int setup;
    int frames;
    int instructions;
    int print_text;
    int trace_tail;
} HeadlessOptions;

static void print_usage(const char *argv0) {
    printf("Usage: %s [options]\n", argv0);
    puts("");
    puts("Options:");
    puts("  --bios PATH        BIOS image path (default: Roms/pc110_bios.bin)");
    puts("  --boot PATH        Boot image or ZIP path");
    puts("  --no-boot          Do not scan default boot media paths");
    puts("  --setup            Enter ROM-backed Easy Setup before running");
    puts("  --frames N         Run N frame ticks (default: 60)");
    puts("  --steps N          Run N CPU instructions before frame ticks");
    puts("  --out PATH         Write the final framebuffer as a BMP");
    puts("  --text             Print the text-mode diagnostic screen");
    puts("  --trace-tail N     Print the last N trace characters");
    puts("  --help             Show this help");
}

static int parse_int(const char *text, int fallback) {
    if (!text || !*text) return fallback;
    char *end = NULL;
    long v = strtol(text, &end, 10);
    if (!end || *end || v < 0 || v > 2147483647L) return fallback;
    return (int)v;
}

static int parse_options(int argc, char **argv, HeadlessOptions *options) {
    memset(options, 0, sizeof(*options));
    options->config.bios_path = "Roms/pc110_bios.bin";
    options->config.use_default_boot_media = 1;
    options->frames = 60;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--bios") == 0 && i + 1 < argc) {
            options->config.bios_path = argv[++i];
        } else if (strcmp(argv[i], "--boot") == 0 && i + 1 < argc) {
            options->config.boot_path = argv[++i];
        } else if (strcmp(argv[i], "--no-boot") == 0) {
            options->config.use_default_boot_media = 0;
        } else if (strcmp(argv[i], "--setup") == 0) {
            options->setup = 1;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            options->frames = parse_int(argv[++i], options->frames);
        } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
            options->instructions = parse_int(argv[++i], options->instructions);
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            options->output_bmp = argv[++i];
        } else if (strcmp(argv[i], "--text") == 0) {
            options->print_text = 1;
        } else if (strcmp(argv[i], "--trace-tail") == 0 && i + 1 < argc) {
            options->trace_tail = parse_int(argv[++i], 0);
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 1;
}

int main(int argc, char **argv) {
    HeadlessOptions options;
    int parsed = parse_options(argc, argv, &options);
    if (parsed <= 0) return parsed == 0 ? 0 : 2;

    PC110Machine *machine = pc110_create();
    if (!machine) {
        fputs("Failed to create PC110 machine\n", stderr);
        return 1;
    }

    char attached_boot[128];
    if (!pc110p_load_default_machine(machine, &options.config, attached_boot, sizeof(attached_boot))) {
        fprintf(stderr, "Failed to load BIOS: %s\n", options.config.bios_path);
        pc110_destroy(machine);
        return 1;
    }

    if (options.setup) {
        pc110_enter_easy_setup(machine);
    }
    if (options.instructions > 0) {
        pc110_cpu_set_trace_mode(machine, 0);
        pc110_cpu_step(machine, options.instructions);
        pc110_cpu_set_trace_mode(machine, 0);
    }
    for (int i = 0; i < options.frames; i++) {
        pc110_run_frame(machine);
    }

    pc110p_print_status(machine, attached_boot);

    if (options.output_bmp) {
        const uint32_t *fb = pc110_get_framebuffer(machine);
        int w = pc110_framebuffer_width();
        int h = pc110_framebuffer_height();
        if (!pc110p_write_bmp(options.output_bmp, fb, w, h)) {
            fprintf(stderr, "Failed to write framebuffer BMP: %s\n", options.output_bmp);
            pc110_destroy(machine);
            return 1;
        }
        printf("Wrote framebuffer: %s (%dx%d)\n", options.output_bmp, w, h);
    }

    if (options.print_text) {
        pc110p_print_text_screen(machine);
    }
    if (options.trace_tail > 0) {
        pc110p_print_trace_tail(machine, (unsigned)options.trace_tail);
    }

    pc110_destroy(machine);
    return 0;
}
