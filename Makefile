CFLAGS += "-m32"

.PHONY: all
all: libSDL-1.3.so.0

.PHONY: clean
clean:
	rm -f libSDL-1.3.so.0

libSDL-1.3.so.0: SDL_compat.c SDL_compat.h
	# -fms-extensions used to 'expand' the SDL_Event union.
	$(CC) -fms-extensions `sdl2-config --libs --cflags` $(LDFLAGS) $(CFLAGS) -shared -fPIC -o libSDL-1.3.so.0 SDL_compat.c
