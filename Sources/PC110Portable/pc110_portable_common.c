#include "pc110_portable_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pc110p_file_exists(const char *path) {
    if (!path || !*path) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

const char *pc110p_basename(const char *path) {
    const char *base = path;
    if (!path) return "";
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return base;
}

static void copy_attached_name(char *attached, unsigned attached_size, const char *path) {
    if (!attached || attached_size == 0u) return;
    const char *name = path ? pc110p_basename(path) : "none";
    snprintf(attached, attached_size, "%s", name);
}

int pc110p_attach_boot_media(PC110Machine *m, const PC110PortableConfig *config, char *attached, unsigned attached_size) {
    if (!m) return 0;
    copy_attached_name(attached, attached_size, "none");

    if (config && config->boot_path && *config->boot_path) {
        if (pc110_attach_boot_image(m, config->boot_path)) {
            copy_attached_name(attached, attached_size, config->boot_path);
            return 1;
        }
        if (pc110_attach_boot_zip(m, config->boot_path)) {
            copy_attached_name(attached, attached_size, config->boot_path);
            return 1;
        }
        return 0;
    }

    if (config && !config->use_default_boot_media) return 0;

    static const char *const zip_paths[] = {
        "Disks/img.ZIP"
    };
    for (unsigned i = 0; i < sizeof(zip_paths) / sizeof(zip_paths[0]); i++) {
        if (pc110p_file_exists(zip_paths[i]) && pc110_attach_boot_zip(m, zip_paths[i])) {
            copy_attached_name(attached, attached_size, zip_paths[i]);
            return 1;
        }
    }

    static const char *const image_paths[] = {
        "Disks/Personaware.PQI",
        "Disks/Disk1.PQI",
        "Disks/disk1.pqi",
        "Disks/disk1.qpi",
        "Disks/Disk1.qpi",
        "Disks/Disk1.QPI",
        "Disks/Disk1.img",
        "Disks/disk.img"
    };
    for (unsigned i = 0; i < sizeof(image_paths) / sizeof(image_paths[0]); i++) {
        if (pc110p_file_exists(image_paths[i]) && pc110_attach_boot_image(m, image_paths[i])) {
            copy_attached_name(attached, attached_size, image_paths[i]);
            return 1;
        }
    }

    return 0;
}

int pc110p_load_default_machine(PC110Machine *m, const PC110PortableConfig *config, char *attached, unsigned attached_size) {
    if (!m) return 0;
    const char *bios_path = (config && config->bios_path && *config->bios_path)
        ? config->bios_path
        : "Roms/pc110_bios.bin";

    int loaded = pc110_load_bios(m, bios_path);
    (void)pc110p_attach_boot_media(m, config, attached, attached_size);
    return loaded;
}

int pc110p_write_bmp(const char *path, const uint32_t *fb, int w, int h) {
    if (!path || !fb || w <= 0 || h <= 0) return 0;

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

void pc110p_print_status(PC110Machine *m, const char *attached_boot) {
    if (!m) return;
    printf("BIOS: %s, size=%u bytes\n",
           pc110_bios_loaded(m) ? "loaded" : "missing",
           (unsigned)pc110_bios_size(m));
    printf("Power MCU: %s, size=%u bytes, rev=%u\n",
           pc110_mcu_firmware_loaded(m) ? "loaded" : "missing",
           (unsigned)pc110_mcu_firmware_size(m),
           (unsigned)pc110_mcu_firmware_revision(m));
    printf("Keyboard MCU: %s, size=%u bytes\n",
           pc110_keyboard_mcu_firmware_loaded(m) ? "loaded" : "missing",
           (unsigned)pc110_keyboard_mcu_firmware_size(m));
    printf("Boot media: %s\n", attached_boot && *attached_boot ? attached_boot : "none");
}

void pc110p_print_text_screen(PC110Machine *m) {
    if (!m) return;
    char buffer[8192];
    pc110_debug_format_text_screen(m, buffer, sizeof(buffer));
    puts(buffer);
}

void pc110p_print_trace_tail(PC110Machine *m, unsigned max_chars) {
    if (!m || max_chars == 0u) return;
    size_t cap = 1024u * 1024u;
    char *buffer = (char *)calloc(cap, 1u);
    if (!buffer) return;

    pc110_trace_copy(m, buffer, cap);
    size_t len = strlen(buffer);
    size_t start = len > max_chars ? len - max_chars : 0u;
    fputs(buffer + start, stdout);
    if (len == 0u || buffer[len - 1u] != '\n') putchar('\n');
    free(buffer);
}
