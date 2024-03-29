/* Most of this file is from SDL 1.3. */

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include <SDL_config.h>

/* This file contains functions for backwards compatibility with SDL ̶1̶.̶2 1.3 */

#include <SDL.h>
#include <SDL_syswm.h>

#include "SDL_compat.h"

static SDL_Window *SDL_VideoWindow = NULL;
static SDL_Surface *SDL_WindowSurface = NULL;
static SDL_Surface *SDL_VideoSurface = NULL;
static SDL_Surface *SDL_ShadowSurface = NULL;
static SDL_Surface *SDL_PublicSurface = NULL;
static SDL_GLContext *SDL_VideoContext = NULL;
static Uint32 SDL_VideoFlags = 0;
static SDL_Rect SDL_VideoViewport;
static char *wm_title = NULL;
static SDL_Surface *SDL_VideoIcon;
static int SDL_enabled_UNICODE = 0;


/* There are few API changes between 2.0 and 1.3, the main one is the removal
 *  of this code, and changes to the mouse wheel event strucutre.
 *
 * Also: 
 *  uint8_t (*SDL_GetMouseState)(int *, int*);
 *
 * became:
 *  uint32_t (*SDL_GetMouseState)(int *, int*);
 *
 * But due to type promotions, these are the same on 32-bit x86 anyway.
 */

/* === Lifted from SDL 1.3 === */

/* 
 * Calculate the pad-aligned scanline width of a surface
 */
int
SDL_CalculatePitch(SDL_Surface * surface)
{
    int pitch;

    /* Surface should be 4-byte aligned for speed */
    pitch = surface->w * surface->format->BytesPerPixel;
    switch (surface->format->BitsPerPixel) {
    case 1:
        pitch = (pitch + 7) / 8;
        break;
    case 4:
        pitch = (pitch + 1) / 2;
        break;
    default:
        break;
    }
    pitch = (pitch + 3) & ~3;   /* 4-byte aligning */
    return (pitch);
}



/* === Lifted from SDL 2.0 === */

/*
 * Calculate an 8-bit (3 red, 3 green, 2 blue) dithered palette of colors
 */
void
SDL_DitherColors(SDL_Color * colors, int bpp)
{
    int i;
    if (bpp != 8)
        return;                 /* only 8bpp supported right now */

    for (i = 0; i < 256; i++) {
        int r, g, b;
        /* map each bit field to the full [0, 255] interval,
           so 0 is mapped to (0, 0, 0) and 255 to (255, 255, 255) */
        r = i & 0xe0;
        r |= r >> 3 | r >> 6;
        colors[i].r = r;
        g = (i << 3) & 0xe0;
        g |= g >> 3 | g >> 6;
        colors[i].g = g;
        b = i & 0x3;
        b |= b << 2;
        b |= b << 4;
        colors[i].b = b;
        colors[i].a = SDL_ALPHA_OPAQUE;
    }
}

/* Yuck -- sorry about this...
 * Needed for SDL_InvalidateMap().
 * Extremely fragile etc... but unchanged in 1.3/2.0
 */

typedef struct
{
    Uint8 *src;
    int src_w, src_h;
    int src_pitch;
    int src_skip;
    Uint8 *dst;
    int dst_w, dst_h;
    int dst_pitch;
    int dst_skip;
    SDL_PixelFormat *src_fmt;
    SDL_PixelFormat *dst_fmt;
    Uint8 *table;
    int flags;
    Uint32 colorkey;
    Uint8 r, g, b, a;
} SDL_BlitInfo;

typedef void (SDLCALL * SDL_BlitFunc) (SDL_BlitInfo * info);

typedef struct
{
    Uint32 src_format;
    Uint32 dst_format;
    int flags;
    int cpu;
    SDL_BlitFunc func;
} SDL_BlitFuncEntry;

/* Blit mapping definition */
typedef struct SDL_BlitMap
{
    SDL_Surface *dst;
    int identity;
    SDL_blit blit;
    void *data;
    SDL_BlitInfo info;

    /* the version count matches the destination; mismatch indicates
       an invalid mapping */
    Uint32 dst_palette_version;
    Uint32 src_palette_version;
} SDL_BlitMap;

void
SDL_InvalidateMap(SDL_BlitMap * map)
{
    if (!map) {
        return;
    }
    if (map->dst) {
        /* Release our reference to the surface - see the note below */
        if (--map->dst->refcount <= 0) {
            SDL_FreeSurface(map->dst);
        }
    }
    map->dst = NULL;
    map->src_palette_version = 0;
    map->dst_palette_version = 0;
    SDL_free(map->info.table);
    map->info.table = NULL;
}

/* =========================== */

const char *
SDL_AudioDriverName(char *namebuf, int maxlen)
{
    const char *name = SDL_GetCurrentAudioDriver();
    if (name) {
        if (namebuf) {
            SDL_strlcpy(namebuf, name, maxlen);
            return namebuf;
        } else {
            return name;
        }
    }
    return NULL;
}

const char *
SDL_VideoDriverName(char *namebuf, int maxlen)
{
    const char *name = SDL_GetCurrentVideoDriver();
    if (name) {
        if (namebuf) {
            SDL_strlcpy(namebuf, name, maxlen);
            return namebuf;
        } else {
            return name;
        }
    }
    return NULL;
}

static int
GetVideoDisplay()
{
    const char *variable = SDL_getenv("SDL_VIDEO_FULLSCREEN_DISPLAY");
    if ( !variable ) {
        variable = SDL_getenv("SDL_VIDEO_FULLSCREEN_HEAD");
    }
    if ( variable ) {
        return SDL_atoi(variable);
    } else {
        return 0;
    }
}

const SDL_VideoInfo *
SDL_GetVideoInfo(void)
{
    static SDL_VideoInfo info;
    SDL_DisplayMode mode;

    /* Memory leak, compatibility code, who cares? */
    if (!info.vfmt && SDL_GetDesktopDisplayMode(GetVideoDisplay(), &mode) == 0) {
        info.vfmt = SDL_AllocFormat(mode.format);
        info.current_w = mode.w;
        info.current_h = mode.h;
    }
    return &info;
}

int
SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags)
{
    int i, actual_bpp = 0;

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        return 0;
    }

    if (!(flags & SDL_FULLSCREEN)) {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(GetVideoDisplay(), &mode);
        return SDL_BITSPERPIXEL(mode.format);
    }

    for (i = 0; i < SDL_GetNumDisplayModes(GetVideoDisplay()); ++i) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(GetVideoDisplay(), i, &mode);
        if (!mode.w || !mode.h || (width == mode.w && height == mode.h)) {
            if (!mode.format) {
                return bpp;
            }
            if (SDL_BITSPERPIXEL(mode.format) >= (Uint32) bpp) {
                actual_bpp = SDL_BITSPERPIXEL(mode.format);
            }
        }
    }
    return actual_bpp;
}

SDL_Rect **
SDL_ListModes(const SDL_PixelFormat * format, Uint32 flags)
{
    int i, nmodes;
    SDL_Rect **modes;

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        return NULL;
    }

    if (!(flags & SDL_FULLSCREEN)) {
        return (SDL_Rect **) (-1);
    }

    if (!format) {
        format = SDL_GetVideoInfo()->vfmt;
    }

    /* Memory leak, but this is a compatibility function, who cares? */
    nmodes = 0;
    modes = NULL;
    for (i = 0; i < SDL_GetNumDisplayModes(GetVideoDisplay()); ++i) {
        SDL_DisplayMode mode;
        int bpp;

        SDL_GetDisplayMode(GetVideoDisplay(), i, &mode);
        if (!mode.w || !mode.h) {
            return (SDL_Rect **) (-1);
        }
        
        /* Copied from src/video/SDL_pixels.c:SDL_PixelFormatEnumToMasks */
        if (SDL_BYTESPERPIXEL(mode.format) <= 2) {
            bpp = SDL_BITSPERPIXEL(mode.format);
        } else {
            bpp = SDL_BYTESPERPIXEL(mode.format) * 8;
        }

        if (bpp != format->BitsPerPixel) {
            continue;
        }
        if (nmodes > 0 && modes[nmodes - 1]->w == mode.w
            && modes[nmodes - 1]->h == mode.h) {
            continue;
        }

        modes = SDL_realloc(modes, (nmodes + 2) * sizeof(*modes));
        if (!modes) {
            return NULL;
        }
        modes[nmodes] = (SDL_Rect *) SDL_malloc(sizeof(SDL_Rect));
        if (!modes[nmodes]) {
            return NULL;
        }
        modes[nmodes]->x = 0;
        modes[nmodes]->y = 0;
        modes[nmodes]->w = mode.w;
        modes[nmodes]->h = mode.h;
        ++nmodes;
    }
    if (modes) {
        modes[nmodes] = NULL;
    }
    return modes;
}

/* === Event handling === */

/* The SDL_MouseWheelEvent structure changed in SDL2.
 * The same ID is used, but the new structure is bigger.
 */

// typedef struct SDL_MouseWheelEvent
// {
//     Uint32 type;        /**< ::SDL_MOUSEWHEEL */
//     Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
//     Uint32 windowID;    /**< The window with mouse focus, if any */
//     Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
//     Sint32 x;           /**< The amount scrolled horizontally, positive to the right and negative to the left */
//     Sint32 y;           /**< The amount scrolled vertically, positive away from the user and negative toward the user */
//     Uint32 direction;   /**< Set to one of the SDL_MOUSEWHEEL_* defines. When FLIPPED the values in X and Y will be opposite. Multiply by -1 to change them back */
// } SDL_MouseWheelEvent;


/* Note: some pre-releases of 1.3 had windowID in this structure. */

typedef struct SDL_MouseWheelEvent_1_3
{
    Uint32 type;
    Uint32 timestamp;
    /* Uint32 windowID; */
    int x;
    int y;
} SDL_MouseWheelEvent_1_3;

/* Lifted from SDL 1.3's SDL_events.h */

typedef struct SDL_ActiveEvent
{
    Uint32 type;
    Uint32 timestamp;
    Uint8 gain;
    Uint8 state;
} SDL_ActiveEvent;

typedef struct SDL_ResizeEvent
{
    Uint32 type;
    Uint32 timestamp;
    int w;
    int h;
} SDL_ResizeEvent;


typedef union SDL_Event_Compat
{
    union SDL_Event;
    SDL_MouseWheelEvent_1_3 wheel_1_3;
    SDL_ActiveEvent active;
    SDL_ResizeEvent resize;
} SDL_Event_Compat;

static int
SDL_CompatEventFilter(void *userdata, SDL_Event * event)
{
    SDL_Event_Compat fake;

    switch (event->type) {
    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_EXPOSED:
            if (!SDL_HasEvent(SDL_VIDEOEXPOSE)) {
                fake.type = SDL_VIDEOEXPOSE;
                SDL_PushEvent((SDL_Event *) &fake);
            }
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_FlushEvent(SDL_VIDEORESIZE);
            /* We don't want to expose that the window width and height will
               be different if we don't get the desired fullscreen mode.
            */
            if (SDL_VideoWindow && !(SDL_GetWindowFlags(SDL_VideoWindow) & SDL_WINDOW_FULLSCREEN)) {
                fake.type = SDL_VIDEORESIZE;
                fake.resize.w = event->window.data1;
                fake.resize.h = event->window.data2;
                SDL_PushEvent((SDL_Event *) &fake);
            }
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 0;
            fake.active.state = SDL_APPACTIVE;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 1;
            fake.active.state = SDL_APPACTIVE;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_ENTER:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 1;
            fake.active.state = SDL_APPMOUSEFOCUS;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 0;
            fake.active.state = SDL_APPMOUSEFOCUS;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 1;
            fake.active.state = SDL_APPINPUTFOCUS;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            fake.type = SDL_ACTIVEEVENT;
            fake.active.gain = 0;
            fake.active.state = SDL_APPINPUTFOCUS;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            fake.type = SDL_QUIT;
            SDL_PushEvent((SDL_Event *) &fake);
            break;
        }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            Uint32 unicode = 0;
            if (event->key.type == SDL_KEYDOWN && event->key.keysym.sym < 256) {
                unicode = event->key.keysym.sym;
                if (unicode >= 'a' && unicode <= 'z') {
                    int shifted = !!(event->key.keysym.mod & KMOD_SHIFT);
                    int capslock = !!(event->key.keysym.mod & KMOD_CAPS);
                    if ((shifted ^ capslock) != 0) {
                        unicode = SDL_toupper(unicode);
                    }
                }
            }
            if (unicode) {
                /* SDL 2.0 renamed 'unicode' to 'unused'.*/
                event->key.keysym.unused = unicode;
            }
            break;
        }
    case SDL_TEXTINPUT:
        {
            /* FIXME: Generate an old style key repeat event if needed */
            //printf("TEXTINPUT: '%s'\n", event->text.text);
            break;
        }
    case SDL_MOUSEMOTION:
        {
            event->motion.x -= SDL_VideoViewport.x;
            event->motion.y -= SDL_VideoViewport.y;
            break;
        }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        {
            event->button.x -= SDL_VideoViewport.x;
            event->button.y -= SDL_VideoViewport.y;
            break;
        }
    case SDL_MOUSEWHEEL:
        {
            Uint8 button;
            int x, y;

            if (event->wheel.y == 0) {
                break;
            }

            SDL_GetMouseState(&x, &y);

            if (event->wheel.y > 0) {
                button = SDL_BUTTON_WHEELUP;
            } else {
                button = SDL_BUTTON_WHEELDOWN;
            }

            fake.button.button = button;
            fake.button.x = x;
            fake.button.y = y;
            fake.button.windowID = event->wheel.windowID;

            fake.type = SDL_MOUSEBUTTONDOWN;
            fake.button.state = SDL_PRESSED;
            SDL_PushEvent((SDL_Event *) &fake);

            fake.type = SDL_MOUSEBUTTONUP;
            fake.button.state = SDL_RELEASED;
            SDL_PushEvent((SDL_Event *) &fake);

            /* Convert to SDL 1.3 style event. */
            /* reuse x and y */
            x = event->wheel.x;
            y = event->wheel.y;
            ((SDL_Event_Compat *) event)->wheel_1_3.x = x;
            ((SDL_Event_Compat *) event)->wheel_1_3.y = y;
            break;
        }

    }
    return 1;
}

static void
GetEnvironmentWindowPosition(int w, int h, int *x, int *y)
{
    int display = GetVideoDisplay();
    const char *window = SDL_getenv("SDL_VIDEO_WINDOW_POS");
    const char *center = SDL_getenv("SDL_VIDEO_CENTERED");
    if (window) {
        if (SDL_sscanf(window, "%d,%d", x, y) == 2) {
            return;
        }
        if (SDL_strcmp(window, "center") == 0) {
            center = window;
        }
    }
    if (center) {
        *x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
        *y = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
    }
}

static void
ClearVideoSurface()
{
    if (SDL_ShadowSurface) {
        SDL_FillRect(SDL_ShadowSurface, NULL,
            SDL_MapRGB(SDL_ShadowSurface->format, 0, 0, 0));
    }
    SDL_FillRect(SDL_WindowSurface, NULL, 0);
    SDL_UpdateWindowSurface(SDL_VideoWindow);
}

static void
SetupScreenSaver(int flags)
{
    const char *env;
    SDL_bool allow_screensaver;

    /* Allow environment override of screensaver disable */
    env = SDL_getenv("SDL_VIDEO_ALLOW_SCREENSAVER");
    if (env) {
        allow_screensaver = SDL_atoi(env) ? SDL_TRUE : SDL_FALSE;
    } else if (flags & SDL_FULLSCREEN) {
        allow_screensaver = SDL_FALSE;
    } else {
        allow_screensaver = SDL_TRUE;
    }
    if (allow_screensaver) {
        SDL_EnableScreenSaver();
    } else {
        SDL_DisableScreenSaver();
    }
}

static int
SDL_ResizeVideoMode(int width, int height, int bpp, Uint32 flags)
{
    int w, h;

    /* We can't resize something we don't have... */
    if (!SDL_VideoSurface) {
        return -1;
    }

    /* We probably have to recreate the window in fullscreen mode */
    if (flags & SDL_FULLSCREEN) {
        return -1;
    }

    /* I don't think there's any change we can gracefully make in flags */
    if (flags != SDL_VideoFlags) {
        return -1;
    }
    if (bpp != SDL_VideoSurface->format->BitsPerPixel) {
        return -1;
    }

    /* Resize the window */
    SDL_GetWindowSize(SDL_VideoWindow, &w, &h);
    if (w != width || h != height) {
        SDL_SetWindowSize(SDL_VideoWindow, width, height);
    }

    /* If we're in OpenGL mode, just resize the stub surface and we're done! */
    if (flags & SDL_OPENGL) {
        SDL_VideoSurface->w = width;
        SDL_VideoSurface->h = height;
        return 0;
    }

    SDL_WindowSurface = SDL_GetWindowSurface(SDL_VideoWindow);
    if (!SDL_WindowSurface) {
        return -1;
    }
    if (SDL_VideoSurface->format != SDL_WindowSurface->format) {
        return -1;
    }
    SDL_VideoSurface->w = width;
    SDL_VideoSurface->h = height;
    SDL_VideoSurface->pixels = SDL_WindowSurface->pixels;
    SDL_VideoSurface->pitch = SDL_WindowSurface->pitch;
    SDL_SetClipRect(SDL_VideoSurface, NULL);

    if (SDL_ShadowSurface) {
        SDL_ShadowSurface->w = width;
        SDL_ShadowSurface->h = height;
        SDL_ShadowSurface->pitch = SDL_CalculatePitch(SDL_ShadowSurface);
        SDL_ShadowSurface->pixels =
            SDL_realloc(SDL_ShadowSurface->pixels,
                        SDL_ShadowSurface->h * SDL_ShadowSurface->pitch);
        SDL_SetClipRect(SDL_ShadowSurface, NULL);
        SDL_InvalidateMap(SDL_ShadowSurface->map);
    } else {
        SDL_PublicSurface = SDL_VideoSurface;
    }

    ClearVideoSurface();

    return 0;
}

SDL_Surface *
SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
    SDL_DisplayMode desktop_mode;
    int display = GetVideoDisplay();
    int window_x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
    int window_y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(display);
    int window_w;
    int window_h;
    Uint32 window_flags;
    Uint32 surface_flags;

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
            return NULL;
        }
    }

    SDL_GetDesktopDisplayMode(display, &desktop_mode);

    if (width == 0) {
        width = desktop_mode.w;
    }
    if (height == 0) {
        height = desktop_mode.h;
    }
    if (bpp == 0) {
        bpp = SDL_BITSPERPIXEL(desktop_mode.format);
    }

    /* See if we can simply resize the existing window and surface */
    if (SDL_ResizeVideoMode(width, height, bpp, flags) == 0) {
        return SDL_PublicSurface;
    }

    /* Destroy existing window */
    SDL_PublicSurface = NULL;
    if (SDL_ShadowSurface) {
        SDL_ShadowSurface->flags &= ~SDL_DONTFREE;
        SDL_FreeSurface(SDL_ShadowSurface);
        SDL_ShadowSurface = NULL;
    }
    if (SDL_VideoSurface) {
        SDL_VideoSurface->flags &= ~SDL_DONTFREE;
        SDL_FreeSurface(SDL_VideoSurface);
        SDL_VideoSurface = NULL;
    }
    if (SDL_VideoContext) {
        /* SDL_GL_MakeCurrent(0, NULL); *//* Doesn't do anything */
        SDL_GL_DeleteContext(SDL_VideoContext);
        SDL_VideoContext = NULL;
    }
    if (SDL_VideoWindow) {
        SDL_GetWindowPosition(SDL_VideoWindow, &window_x, &window_y);
        SDL_DestroyWindow(SDL_VideoWindow);
    }

    /* Set up the event filter */
    if (!SDL_GetEventFilter(NULL, NULL)) {
        SDL_SetEventFilter(SDL_CompatEventFilter, NULL);
    }

    /* Create a new window */
    window_flags = SDL_WINDOW_SHOWN;
    if (flags & SDL_FULLSCREEN) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }
    if (flags & SDL_OPENGL) {
        window_flags |= SDL_WINDOW_OPENGL;
    }
    if (flags & SDL_RESIZABLE) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }
    if (flags & SDL_NOFRAME) {
        window_flags |= SDL_WINDOW_BORDERLESS;
    }
    GetEnvironmentWindowPosition(width, height, &window_x, &window_y);
    SDL_VideoWindow =
        SDL_CreateWindow(wm_title, window_x, window_y, width, height,
                         window_flags);
    if (!SDL_VideoWindow) {
        return NULL;
    }
    SDL_SetWindowIcon(SDL_VideoWindow, SDL_VideoIcon);

    SetupScreenSaver(flags);

    window_flags = SDL_GetWindowFlags(SDL_VideoWindow);
    surface_flags = 0;
    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        surface_flags |= SDL_FULLSCREEN;
    }
    if ((window_flags & SDL_WINDOW_OPENGL) && (flags & SDL_OPENGL)) {
        surface_flags |= SDL_OPENGL;
    }
    if (window_flags & SDL_WINDOW_RESIZABLE) {
        surface_flags |= SDL_RESIZABLE;
    }
    if (window_flags & SDL_WINDOW_BORDERLESS) {
        surface_flags |= SDL_NOFRAME;
    }

    SDL_VideoFlags = flags;

    /* If we're in OpenGL mode, just create a stub surface and we're done! */
    if (flags & SDL_OPENGL) {
        SDL_VideoContext = SDL_GL_CreateContext(SDL_VideoWindow);
        if (!SDL_VideoContext) {
            return NULL;
        }
        if (SDL_GL_MakeCurrent(SDL_VideoWindow, SDL_VideoContext) < 0) {
            return NULL;
        }
        SDL_VideoSurface =
            SDL_CreateRGBSurfaceFrom(NULL, width, height, bpp, 0, 0, 0, 0, 0);
        if (!SDL_VideoSurface) {
            return NULL;
        }
        SDL_VideoSurface->flags |= surface_flags;
        SDL_PublicSurface = SDL_VideoSurface;
        return SDL_PublicSurface;
    }

    /* Create the screen surface */
    SDL_WindowSurface = SDL_GetWindowSurface(SDL_VideoWindow);
    if (!SDL_WindowSurface) {
        return NULL;
    }

    /* Center the public surface in the window surface */
    SDL_GetWindowSize(SDL_VideoWindow, &window_w, &window_h);
    SDL_VideoViewport.x = (window_w - width)/2;
    SDL_VideoViewport.y = (window_h - height)/2;
    SDL_VideoViewport.w = width;
    SDL_VideoViewport.h = height;

    SDL_VideoSurface = SDL_CreateRGBSurfaceFrom(NULL, 0, 0, 32, 0, 0, 0, 0, 0);
    SDL_VideoSurface->flags |= surface_flags;
    SDL_VideoSurface->flags |= SDL_DONTFREE;
    SDL_FreeFormat(SDL_VideoSurface->format);
    SDL_VideoSurface->format = SDL_WindowSurface->format;
    SDL_VideoSurface->format->refcount++;
    SDL_VideoSurface->w = width;
    SDL_VideoSurface->h = height;
    SDL_VideoSurface->pitch = SDL_WindowSurface->pitch;
    SDL_VideoSurface->pixels = (void *)((Uint8 *)SDL_WindowSurface->pixels +
        SDL_VideoViewport.y * SDL_VideoSurface->pitch +
        SDL_VideoViewport.x  * SDL_VideoSurface->format->BytesPerPixel);
    SDL_SetClipRect(SDL_VideoSurface, NULL);

    /* Create a shadow surface if necessary */
    if ((bpp != SDL_VideoSurface->format->BitsPerPixel)
        && !(flags & SDL_ANYFORMAT)) {
        SDL_ShadowSurface =
            SDL_CreateRGBSurface(0, width, height, bpp, 0, 0, 0, 0);
        if (!SDL_ShadowSurface) {
            return NULL;
        }
        SDL_ShadowSurface->flags |= surface_flags;
        SDL_ShadowSurface->flags |= SDL_DONTFREE;

        /* 8-bit SDL_ShadowSurface surfaces report that they have exclusive palette */
        if (SDL_ShadowSurface->format->palette) {
            SDL_ShadowSurface->flags |= SDL_HWPALETTE;
            SDL_DitherColors(SDL_ShadowSurface->format->palette->colors,
                             SDL_ShadowSurface->format->BitsPerPixel);
        }
        SDL_FillRect(SDL_ShadowSurface, NULL,
            SDL_MapRGB(SDL_ShadowSurface->format, 0, 0, 0));
    }
    SDL_PublicSurface =
        (SDL_ShadowSurface ? SDL_ShadowSurface : SDL_VideoSurface);

    ClearVideoSurface();

    /* We're finally done! */
    return SDL_PublicSurface;
}

SDL_Surface *
SDL_GetVideoSurface(void)
{
    return SDL_PublicSurface;
}

int
SDL_SetAlpha(SDL_Surface * surface, Uint32 flag, Uint8 value)
{
    if (flag & SDL_SRCALPHA) {
        /* According to the docs, value is ignored for alpha surfaces */
        if (surface->format->Amask) {
            value = 0xFF;
        }
        SDL_SetSurfaceAlphaMod(surface, value);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    } else {
        SDL_SetSurfaceAlphaMod(surface, 0xFF);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    }
    SDL_SetSurfaceRLE(surface, (flag & SDL_RLEACCEL));

    return 0;
}

SDL_Surface *
SDL_DisplayFormat(SDL_Surface * surface)
{
    SDL_PixelFormat *format;

    if (!SDL_PublicSurface) {
        SDL_SetError("No video mode has been set");
        return NULL;
    }
    format = SDL_PublicSurface->format;

    /* Set the flags appropriate for copying to display surface */
    return SDL_ConvertSurface(surface, format, SDL_RLEACCEL);
}

SDL_Surface *
SDL_DisplayFormatAlpha(SDL_Surface * surface)
{
    SDL_PixelFormat *vf;
    SDL_PixelFormat *format;
    SDL_Surface *converted;
    /* default to ARGB8888 */
    Uint32 amask = 0xff000000;
    Uint32 rmask = 0x00ff0000;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x000000ff;

    if (!SDL_PublicSurface) {
        SDL_SetError("No video mode has been set");
        return NULL;
    }
    vf = SDL_PublicSurface->format;

    switch (vf->BytesPerPixel) {
    case 2:
        /* For XGY5[56]5, use, AXGY8888, where {X, Y} = {R, B}.
           For anything else (like ARGB4444) it doesn't matter
           since we have no special code for it anyway */
        if ((vf->Rmask == 0x1f) &&
            (vf->Bmask == 0xf800 || vf->Bmask == 0x7c00)) {
            rmask = 0xff;
            bmask = 0xff0000;
        }
        break;

    case 3:
    case 4:
        /* Keep the video format, as long as the high 8 bits are
           unused or alpha */
        if ((vf->Rmask == 0xff) && (vf->Bmask == 0xff0000)) {
            rmask = 0xff;
            bmask = 0xff0000;
        }
        break;

    default:
        /* We have no other optimised formats right now. When/if a new
           optimised alpha format is written, add the converter here */
        break;
    }
    format = SDL_AllocFormat(SDL_MasksToPixelFormatEnum(32, rmask,
                                                            gmask,
                                                            bmask,
                                                            amask));
    if (!format) {
        return NULL;
    }
    converted = SDL_ConvertSurface(surface, format, SDL_RLEACCEL);
    SDL_FreeFormat(format);
    return converted;
}

int
SDL_Flip(SDL_Surface * screen)
{
    SDL_UpdateRect(screen, 0, 0, 0, 0);
    return 0;
}

void
SDL_UpdateRect(SDL_Surface * screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
    if (screen) {
        SDL_Rect rect;

        /* Fill the rectangle */
        rect.x = (int) x;
        rect.y = (int) y;
        rect.w = (int) (w ? w : screen->w);
        rect.h = (int) (h ? h : screen->h);
        SDL_UpdateRects(screen, 1, &rect);
    }
}

void
SDL_UpdateRects(SDL_Surface * screen, int numrects, SDL_Rect * rects)
{
    int i;

    if (screen == SDL_ShadowSurface) {
        for (i = 0; i < numrects; ++i) {
            SDL_BlitSurface(SDL_ShadowSurface, &rects[i], SDL_VideoSurface,
                            &rects[i]);
        }

        /* Fall through to video surface update */
        screen = SDL_VideoSurface;
    }
    if (screen == SDL_VideoSurface) {
        if (SDL_VideoViewport.x || SDL_VideoViewport.y) {
            SDL_Rect *stackrects = SDL_stack_alloc(SDL_Rect, numrects);
            SDL_Rect *stackrect;
            const SDL_Rect *rect;
            
            /* Offset all the rectangles before updating */
            for (i = 0; i < numrects; ++i) {
                rect = &rects[i];
                stackrect = &stackrects[i];
                stackrect->x = SDL_VideoViewport.x + rect->x;
                stackrect->y = SDL_VideoViewport.y + rect->y;
                stackrect->w = rect->w;
                stackrect->h = rect->h;
            }
            SDL_UpdateWindowSurfaceRects(SDL_VideoWindow, stackrects, numrects);
            SDL_stack_free(stackrects);
        } else {
            SDL_UpdateWindowSurfaceRects(SDL_VideoWindow, rects, numrects);
        }
    }
}

void
SDL_WM_SetCaption(const char *title, const char *icon)
{
    if (wm_title) {
        SDL_free(wm_title);
    }
    if (title) {
        wm_title = SDL_strdup(title);
    } else {
        wm_title = NULL;
    }
    SDL_SetWindowTitle(SDL_VideoWindow, wm_title);
}

void
SDL_WM_GetCaption(const char **title, const char **icon)
{
    if (title) {
        *title = wm_title;
    }
    if (icon) {
        *icon = "";
    }
}

void
SDL_WM_SetIcon(SDL_Surface * icon, Uint8 * mask)
{
    SDL_VideoIcon = icon;
    ++SDL_VideoIcon->refcount;
}

int
SDL_WM_IconifyWindow(void)
{
    SDL_MinimizeWindow(SDL_VideoWindow);
    return 0;
}

int
SDL_WM_ToggleFullScreen(SDL_Surface * surface)
{
    int length;
    void *pixels;
    Uint8 *src, *dst;
    int row;
    int window_w;
    int window_h;

    if (!SDL_PublicSurface) {
        SDL_SetError("SDL_SetVideoMode() hasn't been called");
        return 0;
    }

    /* Copy the old bits out */
    length = SDL_PublicSurface->w * SDL_PublicSurface->format->BytesPerPixel;
    pixels = SDL_malloc(SDL_PublicSurface->h * length);
    if (pixels && SDL_PublicSurface->pixels) {
        src = (Uint8*)SDL_PublicSurface->pixels;
        dst = (Uint8*)pixels;
        for (row = 0; row < SDL_PublicSurface->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += SDL_PublicSurface->pitch;
            dst += length;
        }
    }

    /* Do the physical mode switch */
    if (SDL_GetWindowFlags(SDL_VideoWindow) & SDL_WINDOW_FULLSCREEN) {
        if (SDL_SetWindowFullscreen(SDL_VideoWindow, 0) < 0) {
            return 0;
        }
        SDL_PublicSurface->flags &= ~SDL_FULLSCREEN;
    } else {
        if (SDL_SetWindowFullscreen(SDL_VideoWindow, 1) < 0) {
            return 0;
        }
        SDL_PublicSurface->flags |= SDL_FULLSCREEN;
    }

    /* Recreate the screen surface */
    SDL_WindowSurface = SDL_GetWindowSurface(SDL_VideoWindow);
    if (!SDL_WindowSurface) {
        /* We're totally hosed... */
        return 0;
    }

    /* Center the public surface in the window surface */
    SDL_GetWindowSize(SDL_VideoWindow, &window_w, &window_h);
    SDL_VideoViewport.x = (window_w - SDL_VideoSurface->w)/2;
    SDL_VideoViewport.y = (window_h - SDL_VideoSurface->h)/2;
    SDL_VideoViewport.w = SDL_VideoSurface->w;
    SDL_VideoViewport.h = SDL_VideoSurface->h;

    /* Do some shuffling behind the application's back if format changes */
    if (SDL_VideoSurface->format->format != SDL_WindowSurface->format->format) {
        if (SDL_ShadowSurface) {
            if (SDL_ShadowSurface->format->format == SDL_WindowSurface->format->format) {
                /* Whee!  We don't need a shadow surface anymore! */
                SDL_VideoSurface->flags &= ~SDL_DONTFREE;
                SDL_FreeSurface(SDL_VideoSurface);
                SDL_free(SDL_ShadowSurface->pixels);
                SDL_VideoSurface = SDL_ShadowSurface;
                SDL_VideoSurface->flags |= SDL_PREALLOC;
                SDL_ShadowSurface = NULL;
            } else {
                /* No problem, just change the video surface format */
                SDL_FreeFormat(SDL_VideoSurface->format);
                SDL_VideoSurface->format = SDL_WindowSurface->format;
                SDL_VideoSurface->format->refcount++;
                SDL_InvalidateMap(SDL_ShadowSurface->map);
            }
        } else {
            /* We can make the video surface the shadow surface */
            SDL_ShadowSurface = SDL_VideoSurface;
            SDL_ShadowSurface->pitch = SDL_CalculatePitch(SDL_ShadowSurface);
            SDL_ShadowSurface->pixels = SDL_malloc(SDL_ShadowSurface->h * SDL_ShadowSurface->pitch);
            if (!SDL_ShadowSurface->pixels) {
                /* Uh oh, we're hosed */
                SDL_ShadowSurface = NULL;
                return 0;
            }
            SDL_ShadowSurface->flags &= ~SDL_PREALLOC;

            SDL_VideoSurface = SDL_CreateRGBSurfaceFrom(NULL, 0, 0, 32, 0, 0, 0, 0, 0);
            SDL_VideoSurface->flags = SDL_ShadowSurface->flags;
            SDL_VideoSurface->flags |= SDL_PREALLOC;
            SDL_FreeFormat(SDL_VideoSurface->format);
            SDL_VideoSurface->format = SDL_WindowSurface->format;
            SDL_VideoSurface->format->refcount++;
            SDL_VideoSurface->w = SDL_ShadowSurface->w;
            SDL_VideoSurface->h = SDL_ShadowSurface->h;
        }
    }

    /* Update the video surface */
    SDL_VideoSurface->pitch = SDL_WindowSurface->pitch;
    SDL_VideoSurface->pixels = (void *)((Uint8 *)SDL_WindowSurface->pixels +
        SDL_VideoViewport.y * SDL_VideoSurface->pitch +
        SDL_VideoViewport.x  * SDL_VideoSurface->format->BytesPerPixel);
    SDL_SetClipRect(SDL_VideoSurface, NULL);

    /* Copy the old bits back */
    if (pixels) {
        src = (Uint8*)pixels;
        dst = (Uint8*)SDL_PublicSurface->pixels;
        for (row = 0; row < SDL_PublicSurface->h; ++row) {
            SDL_memcpy(dst, src, length);
            src += length;
            dst += SDL_PublicSurface->pitch;
        }
        SDL_Flip(SDL_PublicSurface);
        SDL_free(pixels);
    }

    /* We're done! */
    return 1;
}

SDL_GrabMode
SDL_WM_GrabInput(SDL_GrabMode mode)
{
    if (mode != SDL_GRAB_QUERY) {
        SDL_SetWindowGrab(SDL_VideoWindow, mode);
    }
    return (SDL_GrabMode) SDL_GetWindowGrab(SDL_VideoWindow);
}

void
SDL_WarpMouse(Uint16 x, Uint16 y)
{
    SDL_WarpMouseInWindow(SDL_VideoWindow, x, y);
}

Uint8
SDL_GetAppState(void)
{
    Uint8 state = 0;
    Uint32 flags = 0;

    flags = SDL_GetWindowFlags(SDL_VideoWindow);
    if ((flags & SDL_WINDOW_SHOWN) && !(flags & SDL_WINDOW_MINIMIZED)) {
        state |= SDL_APPACTIVE;
    }
    if (flags & SDL_WINDOW_INPUT_FOCUS) {
        state |= SDL_APPINPUTFOCUS;
    }
    if (flags & SDL_WINDOW_MOUSE_FOCUS) {
        state |= SDL_APPMOUSEFOCUS;
    }
    return state;
}

const SDL_version *
SDL_Linked_Version(void)
{
    static SDL_version version;
    SDL_VERSION(&version);
    return &version;
}

int
SDL_SetColors(SDL_Surface * surface, const SDL_Color * colors, int firstcolor,
              int ncolors)
{
    if (SDL_SetPaletteColors
        (surface->format->palette, colors, firstcolor, ncolors) == 0) {
        return 1;
    } else {
        return 0;
    }
}


int
SDL_SetPalette(SDL_Surface * surface, int flags, const SDL_Color * colors,
               int firstcolor, int ncolors)
{
    return SDL_SetColors(surface, colors, firstcolor, ncolors);
}

int
SDL_GetWMInfo(SDL_SysWMinfo * info)
{
    return SDL_GetWindowWMInfo(SDL_VideoWindow, info);
}

// TODO: Implement this.
// As the overlay stuff isn't used in Trine 2 (the reason I'm doing this),
//  I've stubbed it out.

SDL_Overlay *
SDL_CreateYUVOverlay(int w, int h, Uint32 format, SDL_Surface * display)
{
    SDL_SetError("YUV overlays are not implemented in this version of SDL.");
    return NULL;
}

int
SDL_LockYUVOverlay(SDL_Overlay * overlay)
{
    SDL_SetError("YUV overlays are not implemented in this version of SDL.");
    return -1;
}

void
SDL_UnlockYUVOverlay(SDL_Overlay * overlay)
{
    SDL_SetError("YUV overlays are not implemented in this version of SDL.");
    return;
}

int
SDL_DisplayYUVOverlay(SDL_Overlay * overlay, SDL_Rect * dstrect)
{
    SDL_SetError("YUV overlays are not implemented in this version of SDL.");
    return -1;
}

void
SDL_FreeYUVOverlay(SDL_Overlay * overlay)
{
    SDL_SetError("YUV overlays are not implemented in this version of SDL.");
    return;
}

void
SDL_GL_SwapBuffers(void)
{
    SDL_GL_SwapWindow(SDL_VideoWindow);
}

int
SDL_SetGamma(float red, float green, float blue)
{
    Uint16 red_ramp[256];
    Uint16 green_ramp[256];
    Uint16 blue_ramp[256];

    SDL_CalculateGammaRamp(red, red_ramp);
    if (green == red) {
        SDL_memcpy(green_ramp, red_ramp, sizeof(red_ramp));
    } else {
        SDL_CalculateGammaRamp(green, green_ramp);
    }
    if (blue == red) {
        SDL_memcpy(blue_ramp, red_ramp, sizeof(red_ramp));
    } else {
        SDL_CalculateGammaRamp(blue, blue_ramp);
    }
    return SDL_SetWindowGammaRamp(SDL_VideoWindow, red_ramp, green_ramp, blue_ramp);
}

int
SDL_SetGammaRamp(const Uint16 * red, const Uint16 * green, const Uint16 * blue)
{
    return SDL_SetWindowGammaRamp(SDL_VideoWindow, red, green, blue);
}

int
SDL_GetGammaRamp(Uint16 * red, Uint16 * green, Uint16 * blue)
{
    return SDL_GetWindowGammaRamp(SDL_VideoWindow, red, green, blue);
}

int
SDL_EnableKeyRepeat(int delay, int interval)
{
    return 0;
}

void
SDL_GetKeyRepeat(int *delay, int *interval)
{
    if (delay) {
        *delay = SDL_DEFAULT_REPEAT_DELAY;
    }
    if (interval) {
        *interval = SDL_DEFAULT_REPEAT_INTERVAL;
    }
}

int
SDL_EnableUNICODE(int enable)
{
    int previous = SDL_enabled_UNICODE;

    switch (enable) {
    case 1:
        SDL_enabled_UNICODE = 1;
        SDL_StartTextInput();
        break;
    case 0:
        SDL_enabled_UNICODE = 0;
        SDL_StopTextInput();
        break;
    }
    return previous;
}

static Uint32
SDL_SetTimerCallback(Uint32 interval, void* param)
{
    return ((SDL_OldTimerCallback)param)(interval);
}

int
SDL_SetTimer(Uint32 interval, SDL_OldTimerCallback callback)
{
    static SDL_TimerID compat_timer;

    if (compat_timer) {
        SDL_RemoveTimer(compat_timer);
        compat_timer = 0;
    }

    if (interval && callback) {
        compat_timer = SDL_AddTimer(interval, SDL_SetTimerCallback, callback);
        if (!compat_timer) {
            return -1;
        }
    }
    return 0;
}

int
SDL_putenv(const char *_var)
{
    char *ptr = NULL;
    char *var = SDL_strdup(_var);
    if (var == NULL) {
        return -1;  /* we don't set errno. */
    }

    ptr = SDL_strchr(var, '=');
    if (ptr == NULL) {
        SDL_free(var);
        return -1;
    }

    *ptr = '\0';  /* split the string into name and value. */
    SDL_setenv(var, ptr + 1, 1);
    SDL_free(var);
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
