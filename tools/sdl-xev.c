#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define _GNU_SOURCE
#include <dlfcn.h>

/* Note: some pre-releases of 1.3 had windowID */

typedef union SDL_Event {
    uint32_t type;

    struct {
        uint32_t type;
        uint32_t timestamp;
        /* uint32_t windowID; */
        int x;
        int y;
    } wheel_1_3;

    struct {
        uint32_t type;
        uint32_t timestamp;
        uint32_t windowID;
        uint32_t which;
        int x;
        int y;
        uint32_t direction;
    } wheel_2_0;

    uint32_t padding[56];
} SDL_Event;

int (*SDL_Init)(uint32_t flags);
int (*SDL_WaitEvent)(SDL_Event * event);

typedef struct SDL_Window SDL_Window;
SDL_Window * (*SDL_CreateWindow)(const char *title,
                              int x, int y, int w,
                              int h, uint32_t flags);

int (*SDL_SetVideoMode)(int w, int h, int bpp, uint32_t flags);


void load_symbols(const char *lib)
{
    void *sdl = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (sdl == NULL) {
        perror("SDL symbol loading");
        exit(-1);
    }

    SDL_Init = dlsym(sdl, "SDL_Init");
    SDL_WaitEvent = dlsym(sdl, "SDL_WaitEvent");
    SDL_CreateWindow = dlsym(sdl, "SDL_CreateWindow");
    SDL_SetVideoMode = dlsym(sdl, "SDL_SetVideoMode");
}


int main(int argc, char **argv)
{
    int sdl2 = 0;

    switch (argc) {
    case 3:
        if (strcmp(argv[2], "sdl2") == 0) {
            sdl2 = 1;
        }
    case 2:
        break;
    case 1:
    case 0:
        printf("usage: sdl-xev <SDL sofile> [sdl1 or sdl2]\n");
        return -1;
    }

    load_symbols(argv[1]);
    SDL_Init(0xFFFF); /* Equivalent to SDL_INIT_EVERYTHING for 1.3 and 2.0, not for 1.2 */

    if (sdl2) {
        SDL_CreateWindow(
            "sdl-xev", -1, -1,
            512, 512, 0
        );
    } else {
        SDL_SetVideoMode(
            512, 512, 32, 0
        );
    }

    SDL_Event ev;
    while (1) {
        SDL_WaitEvent(&ev);
        printf("event type: %x\n", ev.type);
        if (ev.type == 0x100) {
            break;
        }

        if (ev.type == 0x403) {
            /* 403 is the mouse wheel event type. */
            printf("wheel event as 1.3 - x: %d y: %d\n", ev.wheel_1_3.x, ev.wheel_1_3.y);
            printf("wheel event as 2.0 - x: %d y: %d\n", ev.wheel_2_0.x, ev.wheel_2_0.y);
        }
    }
    return 0;
}
