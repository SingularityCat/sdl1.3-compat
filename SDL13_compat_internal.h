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

#endif
