/*
 * touch_map.h - turns touch fingers into a virtual gamepad state.
 *
 * Pure logic, no SDL/GL, so it builds and tests on the host. The Android
 * glue (port/src/touchpad.c) feeds finger events in and pushes the result
 * into an SDL virtual joystick, so the game's existing pad code reads touch
 * exactly like a physical controller (sticks, triggers, Jinx 1.1 bindings).
 *
 * Virtual pad layout, matching SDL's generated GAMECONTROLLER mapping:
 *   axes:    0 LEFTX, 1 LEFTY, 2 RIGHTX, 3 RIGHTY, 4 LEFTTRIGGER, 5 RIGHTTRIGGER
 *   buttons: 0 A, 1 B, 2 X, 3 Y, 6 START, 13 DPAD_LEFT (see touch_map.c)
 *
 * Roles, decided when a finger goes down:
 *   - on a control from touch_layout  -> that control (joystick or button)
 *   - elsewhere on the left half      -> floating left stick, anchored at touch
 *   - elsewhere on the right half     -> look stick (RIGHTX/RIGHTY), anchored
 *
 * The state is recomputed from the active fingers on every query, so there is
 * no per-control press/release bookkeeping to get out of sync.
 */
#ifndef TOUCH_MAP_H
#define TOUCH_MAP_H

#include "touch_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TM_MAX_FINGERS 10
#define TM_AXIS_COUNT  6
#define TM_BUTTON_COUNT 15

/* Virtual-pad indices, shared with the SDL glue. */
enum {
    TM_AXIS_LEFTX = 0, TM_AXIS_LEFTY, TM_AXIS_RIGHTX, TM_AXIS_RIGHTY,
    TM_AXIS_LEFTTRIGGER, TM_AXIS_RIGHTTRIGGER
};
enum {
    TM_BTN_A = 0, TM_BTN_B = 1, TM_BTN_X = 2, TM_BTN_Y = 3,
    TM_BTN_START = 6, TM_BTN_DPAD_LEFT = 13
};

typedef struct TouchVPad {
    short axis[TM_AXIS_COUNT];           /* -32768..32767 */
    unsigned char button[TM_BUTTON_COUNT];
} TouchVPad;

typedef struct TouchFingerSlot {
    int used;
    long long id;      /* SDL finger id */
    int role;          /* TM_ROLE_* */
    TouchControl ctl;  /* for TM_ROLE_CONTROL */
    float ax, ay;      /* anchor, screen pixels */
    float px, py;      /* current position, screen pixels */
} TouchFingerSlot;

typedef struct TouchMapper {
    TouchLayout layout;
    float sw, sh;      /* screen size in pixels */
    TouchFingerSlot f[TM_MAX_FINGERS];
} TouchMapper;

enum {
    TM_ROLE_NONE = 0,
    TM_ROLE_JOY,       /* left stick */
    TM_ROLE_LOOK,      /* right stick */
    TM_ROLE_CONTROL    /* a button or trigger from the layout */
};

void touchMapInit(TouchMapper *m, const TouchLayout *layout, float sw, float sh);
void touchMapSetScreen(TouchMapper *m, float sw, float sh);
void touchMapDown(TouchMapper *m, long long fingerId, float px, float py);
void touchMapMove(TouchMapper *m, long long fingerId, float px, float py);
void touchMapUp(TouchMapper *m, long long fingerId);

/* Joystick radius in pixels: how far the thumb may travel before it clamps. */
float touchMapStickRadius(const TouchMapper *m);

/* Compute the virtual pad from the currently active fingers. */
void touchMapState(const TouchMapper *m, TouchVPad *out);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_MAP_H */
