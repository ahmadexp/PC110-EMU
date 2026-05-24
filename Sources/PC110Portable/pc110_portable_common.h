#ifndef PC110_PORTABLE_COMMON_H
#define PC110_PORTABLE_COMMON_H

#include <stdint.h>

#include "PC110Core/PC110Core.h"

typedef struct PC110PortableConfig {
    const char *bios_path;
    const char *boot_path;
    int use_default_boot_media;
} PC110PortableConfig;

int pc110p_file_exists(const char *path);
int pc110p_attach_boot_media(PC110Machine *m, const PC110PortableConfig *config, char *attached, unsigned attached_size);
int pc110p_load_default_machine(PC110Machine *m, const PC110PortableConfig *config, char *attached, unsigned attached_size);
int pc110p_write_bmp(const char *path, const uint32_t *fb, int w, int h);
void pc110p_print_status(PC110Machine *m, const char *attached_boot);
void pc110p_print_text_screen(PC110Machine *m);
void pc110p_print_trace_tail(PC110Machine *m, unsigned max_chars);
const char *pc110p_basename(const char *path);

#endif
