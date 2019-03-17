.PHONY: all
all: libSDL-1.3.0.so

.PHONY: clean
clean:
	rm -f libSDL-1.3.0.so

libSDL-1.3.0.so: SDL_compat.c SDL_compat.h
	$(CC) `sdl2-config --libs --cflags` $(LDFLAGS) $(CFLAGS) -shared -o libSDL-1.3.0.so SDL_compat.c
