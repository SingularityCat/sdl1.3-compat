CFLAGS += "-m32"

.PHONY: all
all: sdl-version sdl-xev

.PHONY: clean
clean:
	rm -f sdl-version sdl-xev

sdl-version: sdl-version.c
	gcc $(CFLAGS) $(LDFLAGS) -Og -g sdl-version.c -o sdl-version -ldl

sdl-xev: sdl-xev.c
	gcc $(CFLAGS) $(LDFLAGS) -Og -g sdl-xev.c -o sdl-xev -ldl
