#include "pc110_portable_common.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SDLFrontendOptions {
    PC110PortableConfig config;
    int instructions_per_frame;
} SDLFrontendOptions;

static void print_usage(const char *argv0) {
    printf("Usage: %s [options]\n", argv0);
    puts("");
    puts("Options:");
    puts("  --bios PATH        BIOS image path (default: Roms/pc110_bios.bin)");
    puts("  --boot PATH        Boot image or ZIP path");
    puts("  --no-boot          Do not scan default boot media paths");
    puts("  --ips N            Approximate instructions per rendered frame (default: 266666)");
    puts("  --help             Show this help");
}

static int parse_int(const char *text, int fallback) {
    if (!text || !*text) return fallback;
    char *end = NULL;
    long v = strtol(text, &end, 10);
    if (!end || *end || v < 0 || v > 2147483647L) return fallback;
    return (int)v;
}

static int parse_options(int argc, char **argv, SDLFrontendOptions *options) {
    memset(options, 0, sizeof(*options));
    options->config.bios_path = "Roms/pc110_bios.bin";
    options->config.use_default_boot_media = 1;
    options->instructions_per_frame = 266666;

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
        } else if (strcmp(argv[i], "--ips") == 0 && i + 1 < argc) {
            options->instructions_per_frame = parse_int(argv[++i], options->instructions_per_frame);
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }
    return 1;
}

static int sdl_scancode_to_mac(SDL_Scancode code, uint16_t *mac_key_code) {
    if (!mac_key_code) return 0;
    switch (code) {
        case SDL_SCANCODE_A: *mac_key_code = 0; return 1;
        case SDL_SCANCODE_S: *mac_key_code = 1; return 1;
        case SDL_SCANCODE_D: *mac_key_code = 2; return 1;
        case SDL_SCANCODE_F: *mac_key_code = 3; return 1;
        case SDL_SCANCODE_H: *mac_key_code = 4; return 1;
        case SDL_SCANCODE_G: *mac_key_code = 5; return 1;
        case SDL_SCANCODE_Z: *mac_key_code = 6; return 1;
        case SDL_SCANCODE_X: *mac_key_code = 7; return 1;
        case SDL_SCANCODE_C: *mac_key_code = 8; return 1;
        case SDL_SCANCODE_V: *mac_key_code = 9; return 1;
        case SDL_SCANCODE_B: *mac_key_code = 11; return 1;
        case SDL_SCANCODE_Q: *mac_key_code = 12; return 1;
        case SDL_SCANCODE_W: *mac_key_code = 13; return 1;
        case SDL_SCANCODE_E: *mac_key_code = 14; return 1;
        case SDL_SCANCODE_R: *mac_key_code = 15; return 1;
        case SDL_SCANCODE_Y: *mac_key_code = 16; return 1;
        case SDL_SCANCODE_T: *mac_key_code = 17; return 1;
        case SDL_SCANCODE_1: *mac_key_code = 18; return 1;
        case SDL_SCANCODE_2: *mac_key_code = 19; return 1;
        case SDL_SCANCODE_3: *mac_key_code = 20; return 1;
        case SDL_SCANCODE_4: *mac_key_code = 21; return 1;
        case SDL_SCANCODE_6: *mac_key_code = 22; return 1;
        case SDL_SCANCODE_5: *mac_key_code = 23; return 1;
        case SDL_SCANCODE_EQUALS: *mac_key_code = 24; return 1;
        case SDL_SCANCODE_9: *mac_key_code = 25; return 1;
        case SDL_SCANCODE_7: *mac_key_code = 26; return 1;
        case SDL_SCANCODE_MINUS: *mac_key_code = 27; return 1;
        case SDL_SCANCODE_8: *mac_key_code = 28; return 1;
        case SDL_SCANCODE_0: *mac_key_code = 29; return 1;
        case SDL_SCANCODE_RIGHTBRACKET: *mac_key_code = 30; return 1;
        case SDL_SCANCODE_O: *mac_key_code = 31; return 1;
        case SDL_SCANCODE_U: *mac_key_code = 32; return 1;
        case SDL_SCANCODE_LEFTBRACKET: *mac_key_code = 33; return 1;
        case SDL_SCANCODE_I: *mac_key_code = 34; return 1;
        case SDL_SCANCODE_P: *mac_key_code = 35; return 1;
        case SDL_SCANCODE_RETURN: *mac_key_code = 36; return 1;
        case SDL_SCANCODE_L: *mac_key_code = 37; return 1;
        case SDL_SCANCODE_J: *mac_key_code = 38; return 1;
        case SDL_SCANCODE_APOSTROPHE: *mac_key_code = 39; return 1;
        case SDL_SCANCODE_K: *mac_key_code = 40; return 1;
        case SDL_SCANCODE_SEMICOLON: *mac_key_code = 41; return 1;
        case SDL_SCANCODE_BACKSLASH: *mac_key_code = 42; return 1;
        case SDL_SCANCODE_COMMA: *mac_key_code = 43; return 1;
        case SDL_SCANCODE_SLASH: *mac_key_code = 44; return 1;
        case SDL_SCANCODE_N: *mac_key_code = 45; return 1;
        case SDL_SCANCODE_M: *mac_key_code = 46; return 1;
        case SDL_SCANCODE_PERIOD: *mac_key_code = 47; return 1;
        case SDL_SCANCODE_TAB: *mac_key_code = 48; return 1;
        case SDL_SCANCODE_SPACE: *mac_key_code = 49; return 1;
        case SDL_SCANCODE_GRAVE: *mac_key_code = 50; return 1;
        case SDL_SCANCODE_ESCAPE: *mac_key_code = 53; return 1;
        case SDL_SCANCODE_KP_ENTER: *mac_key_code = 76; return 1;
        case SDL_SCANCODE_F1: *mac_key_code = 122; return 1;
        case SDL_SCANCODE_F2: *mac_key_code = 120; return 1;
        case SDL_SCANCODE_F3: *mac_key_code = 99; return 1;
        case SDL_SCANCODE_F4: *mac_key_code = 118; return 1;
        case SDL_SCANCODE_F5: *mac_key_code = 96; return 1;
        case SDL_SCANCODE_F6: *mac_key_code = 97; return 1;
        case SDL_SCANCODE_F7: *mac_key_code = 98; return 1;
        case SDL_SCANCODE_F8: *mac_key_code = 100; return 1;
        case SDL_SCANCODE_F9: *mac_key_code = 101; return 1;
        case SDL_SCANCODE_F10: *mac_key_code = 109; return 1;
        case SDL_SCANCODE_F11: *mac_key_code = 103; return 1;
        case SDL_SCANCODE_F12: *mac_key_code = 111; return 1;
        case SDL_SCANCODE_LEFT: *mac_key_code = 123; return 1;
        case SDL_SCANCODE_RIGHT: *mac_key_code = 124; return 1;
        case SDL_SCANCODE_DOWN: *mac_key_code = 125; return 1;
        case SDL_SCANCODE_UP: *mac_key_code = 126; return 1;
        case SDL_SCANCODE_BACKSPACE: *mac_key_code = 51; return 1;
        case SDL_SCANCODE_DELETE: *mac_key_code = 117; return 1;
        default: return 0;
    }
}

static void window_to_emulator(SDL_Window *window, int wx, int wy, int *x, int *y) {
    int ww = 1;
    int wh = 1;
    SDL_GetWindowSize(window, &ww, &wh);
    int ew = pc110_framebuffer_width();
    int eh = pc110_framebuffer_height();
    double scale_x = (double)ww / (double)ew;
    double scale_y = (double)wh / (double)eh;
    double scale = scale_x < scale_y ? scale_x : scale_y;
    int draw_w = (int)((double)ew * scale);
    int draw_h = (int)((double)eh * scale);
    int ox = (ww - draw_w) / 2;
    int oy = (wh - draw_h) / 2;
    int ex = (int)((double)(wx - ox) / scale);
    int ey = (int)((double)(wy - oy) / scale);
    if (ex < 0) ex = 0;
    if (ey < 0) ey = 0;
    if (ex >= ew) ex = ew - 1;
    if (ey >= eh) ey = eh - 1;
    *x = ex;
    *y = ey;
}

int main(int argc, char **argv) {
    SDLFrontendOptions options;
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
    pc110p_print_status(machine, attached_boot);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        pc110_destroy(machine);
        return 1;
    }

    int fb_w = pc110_framebuffer_width();
    int fb_h = pc110_framebuffer_height();
    SDL_Window *window = SDL_CreateWindow(
        "PC110 EMU",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        fb_w * 2,
        fb_h * 2,
        SDL_WINDOW_RESIZABLE
    );
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    SDL_Texture *texture = renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, fb_w, fb_h) : NULL;
    if (!window || !renderer || !texture) {
        fprintf(stderr, "SDL window setup failed: %s\n", SDL_GetError());
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        pc110_destroy(machine);
        return 1;
    }

    SDL_StartTextInput();

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_TEXTINPUT:
                    for (const char *p = event.text.text; *p; p++) {
                        unsigned char ch = (unsigned char)*p;
                        if (ch >= 0x20u && ch <= 0x7Eu) {
                            pc110_key_ascii(machine, ch);
                        }
                    }
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    uint16_t mac_key_code = 0;
                    if (sdl_scancode_to_mac(event.key.keysym.scancode, &mac_key_code)) {
                        if (event.type == SDL_KEYDOWN) {
                            pc110_key_down(machine, mac_key_code);
                        } else {
                            pc110_key_up(machine, mac_key_code);
                        }
                    }
                    break;
                }
                case SDL_MOUSEMOTION: {
                    int x = 0;
                    int y = 0;
                    window_to_emulator(window, event.motion.x, event.motion.y, &x, &y);
                    pc110_mouse_move(machine, x, y);
                    break;
                }
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    int x = 0;
                    int y = 0;
                    window_to_emulator(window, event.button.x, event.button.y, &x, &y);
                    int button = event.button.button == SDL_BUTTON_RIGHT ? 1 : 0;
                    if (event.type == SDL_MOUSEBUTTONDOWN) {
                        pc110_mouse_down(machine, x, y, button);
                    } else {
                        pc110_mouse_up(machine, x, y, button);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (options.instructions_per_frame > 0) {
            pc110_cpu_set_trace_mode(machine, 0);
            pc110_cpu_step(machine, options.instructions_per_frame);
            pc110_cpu_set_trace_mode(machine, 0);
        } else {
            pc110_run_frame(machine);
        }
        pc110_run_frame(machine);

        const uint32_t *fb = pc110_get_framebuffer(machine);
        SDL_UpdateTexture(texture, NULL, fb, fb_w * (int)sizeof(uint32_t));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    pc110_destroy(machine);
    return 0;
}
