sdl1.3-compat
=============

Dreadful SDL 1.3 compatibility code, ripped out of SDL 1.3.

This is the SDL_compat module that existed in the SDL 1.3 codebase,
 modified to avoid references to internal or now absent SDL 2.0 functions.

It produces a shared library, linked against SDL 2.0, consisting of the
 symbols missing from 2.0 compared to 1.3.

It also translates `struct SDL_MouseWheelEvent` using an event filter.


Bugs
----
 * SDL_MouseWheelEvent structures will not be translated unless the
   compatibility event filter is used.
 * The YUV overlay support is just a load of stubs that always error.
 
 
Why the hell did you...?
------------------------
Trine 2 shipped compiled against some build of SDL 1.3.
SDL 1.2, 1.3 and early 2.0 didn't handle fullscreen well with multiple monitors.

So I 'ported' it to SDL 2.0 like this.
