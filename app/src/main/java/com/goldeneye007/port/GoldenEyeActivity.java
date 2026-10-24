package com.goldeneye007.port;

import org.libsdl.app.SDLActivity;

/**
 * Entry point. All game logic is native (libmain.so, built from the port's C
 * sources). SDLActivity does the window, input and GL surface plumbing.
 */
public class GoldenEyeActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        // SDL2 is built as a dependency of libmain via CMake.
        return new String[] { "SDL2", "main" };
    }
}
