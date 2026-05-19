#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PC110Core/PC110Core.h"

static int attach_first_boot_image(PC110Machine *m) {
    if (pc110_attach_boot_image(m, "Disks/Disk1.img")) return 1;
    if (pc110_attach_boot_image(m, "Disks/disk.img")) return 1;
    return 0;
}

static int write_bmp(const char *path, const uint32_t *fb, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    uint32_t pixel_bytes = (uint32_t)(w * h * 4);
    uint32_t file_size = 54u + pixel_bytes;
    unsigned char header[54] = {
        'B', 'M',
        (unsigned char)(file_size), (unsigned char)(file_size >> 8),
        (unsigned char)(file_size >> 16), (unsigned char)(file_size >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0,
        40, 0, 0, 0,
        (unsigned char)(w), (unsigned char)(w >> 8), (unsigned char)(w >> 16), (unsigned char)(w >> 24),
        (unsigned char)(h), (unsigned char)(h >> 8), (unsigned char)(h >> 16), (unsigned char)(h >> 24),
        1, 0,
        32, 0,
        0, 0, 0, 0,
        (unsigned char)(pixel_bytes), (unsigned char)(pixel_bytes >> 8),
        (unsigned char)(pixel_bytes >> 16), (unsigned char)(pixel_bytes >> 24),
        0x13, 0x0B, 0, 0,
        0x13, 0x0B, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    fwrite(header, 1, sizeof(header), f);

    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            uint32_t p = fb[y * w + x];
            unsigned char bgra[4] = {
                (unsigned char)(p & 0xFFu),
                (unsigned char)((p >> 8) & 0xFFu),
                (unsigned char)((p >> 16) & 0xFFu),
                (unsigned char)((p >> 24) & 0xFFu)
            };
            fwrite(bgra, 1, 4, f);
        }
    }

    fclose(f);
    return 1;
}

static void print_region_summary(PC110Machine *m, uint32_t base, uint32_t len, const char *name) {
    unsigned nonzero = 0;
    unsigned ff = 0;
    unsigned unique[256] = {0};
    for (uint32_t i = 0; i < len; i++) {
        uint8_t v = pc110_mem_read8(m, base + i);
        if (v) nonzero++;
        if (v == 0xFFu) ff++;
        unique[v] = 1;
    }
    unsigned unique_count = 0;
    for (unsigned i = 0; i < 256; i++) unique_count += unique[i];
    printf("%s base=%05X len=%u nonzero=%u ff=%u unique=%u sample:",
           name, base, len, nonzero, ff, unique_count);
    for (uint32_t i = 0; i < 32 && i < len; i++) {
        printf(" %02X", pc110_mem_read8(m, base + i));
    }
    printf(" first-nz:");
    unsigned shown = 0;
    for (uint32_t i = 0; i < len && shown < 16; i++) {
        uint8_t v = pc110_mem_read8(m, base + i);
        if (v) {
            printf(" +%04X=%02X", i, v);
            shown++;
        }
    }
    putchar('\n');
}

static void print_bytes(PC110Machine *m, uint32_t base, uint32_t len, const char *name) {
    printf("%s base=%05X:", name, base);
    for (uint32_t i = 0; i < len; i++) {
        printf(" %02X", pc110_mem_read8(m, base + i));
    }
    putchar('\n');
}

static void print_vga_plane_summary(PC110Machine *m) {
    pc110_io_write8(m, 0x03CEu, 0x05u);
    pc110_io_write8(m, 0x03CFu, 0x00u);
    for (unsigned plane = 0; plane < 4; plane++) {
        pc110_io_write8(m, 0x03CEu, 0x04u);
        pc110_io_write8(m, 0x03CFu, (uint8_t)plane);
        char name[32];
        snprintf(name, sizeof(name), "A000 plane %u", plane);
        print_region_summary(m, 0x000A0000u, 0x10000u, name);
        snprintf(name, sizeof(name), "A000 p%u icon", plane);
        print_bytes(m, 0x000A3200u, 0x80u, name);
    }
}

static void print_frame_summary(const uint32_t *fb, int w, int h) {
    uint32_t colors[16] = {0};
    unsigned counts[16] = {0};
    unsigned used = 0;
    for (int i = 0; i < w * h; i++) {
        uint32_t p = fb[i];
        unsigned slot = 0;
        for (; slot < used; slot++) {
            if (colors[slot] == p) break;
        }
        if (slot == used) {
            if (used < 16) {
                colors[used] = p;
                counts[used] = 1;
                used++;
            }
        } else {
            counts[slot]++;
        }
    }

    printf("frame colors used<=16=%u", used);
    for (unsigned i = 0; i < used; i++) {
        printf(" #%08X=%u", colors[i], counts[i]);
    }
    putchar('\n');
}

static int mac_key_code_for_char(char ch, uint16_t *mac_key_code) {
    if (!mac_key_code) return 0;

    switch (ch) {
        case 'a': case 'A': *mac_key_code = 0; return 1;
        case 'b': case 'B': *mac_key_code = 11; return 1;
        case 'c': case 'C': *mac_key_code = 8; return 1;
        case 'd': case 'D': *mac_key_code = 2; return 1;
        case 'e': case 'E': *mac_key_code = 14; return 1;
        case 'f': case 'F': *mac_key_code = 3; return 1;
        case 'g': case 'G': *mac_key_code = 5; return 1;
        case 'h': case 'H': *mac_key_code = 4; return 1;
        case 'i': case 'I': *mac_key_code = 34; return 1;
        case 'j': case 'J': *mac_key_code = 38; return 1;
        case 'k': case 'K': *mac_key_code = 40; return 1;
        case 'l': case 'L': *mac_key_code = 37; return 1;
        case 'm': case 'M': *mac_key_code = 46; return 1;
        case 'n': case 'N': *mac_key_code = 45; return 1;
        case 'o': case 'O': *mac_key_code = 31; return 1;
        case 'p': case 'P': *mac_key_code = 35; return 1;
        case 'q': case 'Q': *mac_key_code = 12; return 1;
        case 'r': case 'R': *mac_key_code = 15; return 1;
        case 's': case 'S': *mac_key_code = 1; return 1;
        case 't': case 'T': *mac_key_code = 17; return 1;
        case 'u': case 'U': *mac_key_code = 32; return 1;
        case 'v': case 'V': *mac_key_code = 9; return 1;
        case 'w': case 'W': *mac_key_code = 13; return 1;
        case 'x': case 'X': *mac_key_code = 7; return 1;
        case 'y': case 'Y': *mac_key_code = 16; return 1;
        case 'z': case 'Z': *mac_key_code = 6; return 1;
        case '1': *mac_key_code = 18; return 1;
        case '2': *mac_key_code = 19; return 1;
        case '3': *mac_key_code = 20; return 1;
        case '4': *mac_key_code = 21; return 1;
        case '5': *mac_key_code = 23; return 1;
        case '6': *mac_key_code = 22; return 1;
        case '7': *mac_key_code = 26; return 1;
        case '8': *mac_key_code = 28; return 1;
        case '9': *mac_key_code = 25; return 1;
        case '0': *mac_key_code = 29; return 1;
        case '-': *mac_key_code = 27; return 1;
        case '=': *mac_key_code = 24; return 1;
        case '[': *mac_key_code = 33; return 1;
        case ']': *mac_key_code = 30; return 1;
        case '\\': *mac_key_code = 42; return 1;
        case ';': *mac_key_code = 41; return 1;
        case '\'': *mac_key_code = 39; return 1;
        case ',': *mac_key_code = 43; return 1;
        case '.': *mac_key_code = 47; return 1;
        case '/': *mac_key_code = 44; return 1;
        case '`': *mac_key_code = 50; return 1;
        case ' ': *mac_key_code = 49; return 1;
        default: return 0;
    }
}

static int press_text(PC110Machine *m, const char *text) {
    if (!m || !text) return 0;

    int pressed = 0;
    for (const char *p = text; *p; p++) {
        uint16_t mac_key_code = 0;
        if (!mac_key_code_for_char(*p, &mac_key_code)) return 0;
        pc110_key_down(m, mac_key_code);
        pressed++;
    }
    return pressed > 0;
}

static void press_named_key(PC110Machine *m, const char *name) {
    if (!m || !name) return;
    if (strcmp(name, "none") == 0) return;
    if (strcmp(name, "enter") == 0) pc110_key_down(m, 36);
    else if (strcmp(name, "esc") == 0) pc110_key_down(m, 53);
    else if (strcmp(name, "tab") == 0) pc110_key_down(m, 48);
    else if (strcmp(name, "space") == 0) pc110_key_down(m, 49);
    else if (strcmp(name, "left") == 0) pc110_key_down(m, 123);
    else if (strcmp(name, "right") == 0) pc110_key_down(m, 124);
    else if (strcmp(name, "down") == 0) pc110_key_down(m, 125);
    else if (strcmp(name, "up") == 0) pc110_key_down(m, 126);
    else if (strcmp(name, "f1") == 0) pc110_key_down(m, 122);
    else if (strcmp(name, "f2") == 0) pc110_key_down(m, 120);
    else if (strcmp(name, "f3") == 0) pc110_key_down(m, 99);
    else if (strcmp(name, "f4") == 0) pc110_key_down(m, 118);
    else if (strcmp(name, "f5") == 0) pc110_key_down(m, 96);
    else if (strcmp(name, "f6") == 0) pc110_key_down(m, 97);
    else if (strcmp(name, "f7") == 0) pc110_key_down(m, 98);
    else if (strcmp(name, "f8") == 0) pc110_key_down(m, 100);
    else if (strcmp(name, "f9") == 0) pc110_key_down(m, 101);
    else if (strcmp(name, "f10") == 0) pc110_key_down(m, 109);
    else press_text(m, name);
}

static void press_key_sequence(PC110Machine *m, const char *sequence) {
    if (!m || !sequence || !*sequence) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", sequence);
    char *tok = strtok(buf, ",");
    while (tok) {
        press_named_key(m, tok);
        tok = strtok(NULL, ",");
    }
}

int main(int argc, char **argv) {
    const char *out = (argc > 1) ? argv[1] : "/private/tmp/pc110_frame.bmp";
    const char *mode = (argc > 2) ? argv[2] : "setup";
    int steps = (argc > 3) ? atoi(argv[3]) : 0;
    const char *keys = (argc > 4) ? argv[4] : "";
    int post_key_steps = (argc > 5) ? atoi(argv[5]) : 0;
    int trace_post_key = (argc > 6) ? (strcmp(argv[6], "trace") == 0) : 0;

    PC110Machine *m = pc110_create();
    if (!m) return 2;
    if (!pc110_load_bios(m, "Roms/pc110_bios.bin")) return 3;
    attach_first_boot_image(m);
    pc110_cpu_set_trace_mode(m, 0);

    if (strcmp(mode, "setup") == 0) {
        pc110_enter_easy_setup(m);
    } else {
        pc110_cpu_step(m, steps > 0 ? steps : 4000000);
        pc110_run_frame(m);
    }
    if (steps > 0) {
        pc110_cpu_step(m, steps);
        pc110_run_frame(m);
    }
    press_key_sequence(m, keys);
    if (post_key_steps > 0) {
        if (trace_post_key) {
            pc110_trace_clear(m);
            pc110_cpu_set_trace_mode(m, 1);
        }
        pc110_cpu_step(m, post_key_steps);
        pc110_cpu_set_trace_mode(m, 0);
        pc110_run_frame(m);
    }

    const uint32_t *fb = pc110_get_framebuffer(m);
    int ok = write_bmp(out, fb, pc110_framebuffer_width(), pc110_framebuffer_height());
    print_frame_summary(fb, pc110_framebuffer_width(), pc110_framebuffer_height());

    char state[12000];
    pc110_cpu_format_state(m, state, sizeof(state));
    puts(state);
    print_region_summary(m, 0x000A0000u, 0x10000u, "A000 graphics");
    print_vga_plane_summary(m);
    print_region_summary(m, 0x00010000u, 0x02000u, "1000 work buffer");
    print_region_summary(m, 0x00020000u, 0x02000u, "2000 glyph cache");
    print_region_summary(m, 0x000B0000u, 0x08000u, "B000 mono");
    print_region_summary(m, 0x000B8000u, 0x08000u, "B800 color");
    print_bytes(m, 0x00020000u, 0x80u, "glyph cache head");
    print_bytes(m, 0x00020000u, 0x0800u, "glyph cache all");
    print_bytes(m, 0x00057CDAu, 0x60u, "table 7CDA");
    print_bytes(m, 0x00057D30u, 0x50u, "table 7D30");
    print_bytes(m, 0x00059A52u, 0x50u, "table 9A52");
    if (trace_post_key) {
        char *trace = (char *)malloc(1200000u);
        if (trace) {
            pc110_trace_copy(m, trace, 1200000u);
            puts(trace);
            free(trace);
        }
    }
    printf("frame=%s ok=%d\n", out, ok);

    pc110_destroy(m);
    return ok ? 0 : 1;
}
