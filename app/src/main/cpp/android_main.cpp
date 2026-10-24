/*
 * Android bootstrap. SDL calls SDL_main on the native thread once the activity
 * is up. The real game entry point is the port's main(); this file only exists
 * so the link has a symbol and so the touch overlay has one place to hook in.
 * TODO(port): call the port's game main here and route touch events to
 * touchLayoutHit() from touch_layout.h.
 */
#include <SDL.h>

extern "C" int SDL_main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}
