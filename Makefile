.PHONY: all
all: libSDL-1.3.so.0

.PHONY: clean
clean:
	rm -f libSDL-1.3.so.0

libSDL-1.3.so.0: SDL_compat.c SDL_compat.h
	# -fms-extensions used to 'expand' the SDL_Event union.
	# -Wl,--no-as-needed disables LTO - useful if we're trying to tack a bit on to SDL 2.0.
	$(CC) -fms-extensions -Wl,--no-as-needed `sdl2-config --libs --cflags` $(LDFLAGS) $(CFLAGS) -shared -o libSDL-1.3.so.0 SDL_compat.c
