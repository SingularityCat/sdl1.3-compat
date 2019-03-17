#ifndef _SDL13_COMPAT_INTERNAL_H
#define _SDL13_COMPAT_INTERNAL_H

#include <SDL_surface.h>
#include <SDL_pixels.h>

/* This file contains the parts of SDL 2.0 internal headers that were used by
 *  SDL 1.3's compat module.
 */

/* #include "video/SDL_sysvideo.h" */
typedef struct SDL_VideoDevice SDL_VideoDevice;
extern SDL_VideoDevice *SDL_GetVideoDevice(void);

/* #include "video/SDL_pixel_c.h" */
typedef struct SDL_BlitMap SDL_BlitMap;
extern void SDL_InvalidateMap(SDL_BlitMap * map);
extern void SDL_DitherColors(SDL_Color * colors, int bpp);

#if 0

/* #include "render/SDL_yuv_sw_c.h" */
struct SDL_SW_YUVTexture
{
    Uint32 format;
    Uint32 target_format;
    int w, h;
    Uint8 *pixels;

    /* These are just so we don't have to allocate them separately */
    Uint16 pitches[3];
    Uint8 *planes[3];

    /* This is a temporary surface in case we have to stretch copy */
    SDL_Surface *stretch;
    SDL_Surface *display;
};

typedef struct SDL_SW_YUVTexture SDL_SW_YUVTexture;

SDL_SW_YUVTexture *SDL_SW_CreateYUVTexture(Uint32 format, int w, int h);
void SDL_SW_DestroyYUVTexture(SDL_SW_YUVTexture * swdata);
int SDL_SW_LockYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                          void **pixels, int *pitch);
void SDL_SW_UnlockYUVTexture(SDL_SW_YUVTexture * swdata);
int SDL_SW_CopyYUVToRGB(SDL_SW_YUVTexture * swdata, const SDL_Rect * srcrect,
                        Uint32 target_format, int w, int h, void *pixels,
                        int pitch);

#endif

#endif
