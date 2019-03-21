#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define _GNU_SOURCE
#include <dlfcn.h>

typedef struct SDL_version
{
    uint8_t major;  /* major version */
    uint8_t minor;  /* minor version */
    uint8_t patch;  /* update version */
} SDL_version;

void (*SDL_GetVersion)(SDL_version * ver);

void load_symbols(const char *lib)
{
    void *sdl = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (sdl == NULL) {
        perror("SDL symbol loading");
        exit(-1);
    }

    SDL_GetVersion = dlsym(sdl, "SDL_GetVersion");
}

int main(int argc, char **argv)
{
    switch (argc) {
    case 2:
        break;
    case 1:
    case 0:
        printf("usage: sdl-version <SDL sofile>\n");
        return -1;
    }

    load_symbols(argv[1]);

    SDL_version version;
    SDL_GetVersion(&version);
    printf("%s is at SDL version %d.%d.%d\n",
        argv[1],
        version.major, version.minor, version.patch);
    return 0;
}
