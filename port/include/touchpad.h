/*
 * touchpad.h - Android touch controls.
 *
 * Touch becomes an SDL virtual gamepad, so the game's existing pad code
 * (sticks, triggers, Jinx 1.1 bindings) reads it like a real controller.
 * The layout and finger mapping live in app/src/main/cpp/touch_map.c; this
 * module wires them to SDL and draws the overlay. Android-only; on PC these
 * functions are not built.
 */
#ifndef PORT_TOUCHPAD_H
#define PORT_TOUCHPAD_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Attach the virtual gamepad and disable touch-to-mouse emulation. Call once,
 * before the pads are opened (input.c inputInit). */
void touchpadInit(void);

/* Feed one SDL touch event (SDL_FINGERDOWN, SDL_FINGERMOTION, SDL_FINGERUP).
 * Called from the host event pump. */
void touchpadHandleEvent(const SDL_Event *ev);

/* Draw the overlay over the finished frame. Render thread, GL context current,
 * before the buffer swap. Saves and restores the GL state it touches. */
void touchpadDraw(int drawableW, int drawableH);

#ifdef __cplusplus
}
#endif

#endif /* PORT_TOUCHPAD_H */
