/*
 * touch_layout.h - on-screen touch control layout for the Android build.
 *
 * Pure logic, no SDL/GL: positions are stored normalized (0..1 of the screen),
 * so the same saved layout works on any phone, ultrawide or not. Rendering and
 * touch dispatch live elsewhere and only ask this module where things are.
 */
#ifndef TOUCH_LAYOUT_H
#define TOUCH_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TouchControl {
    TC_JOYSTICK = 0,
    TC_FIRE,
    TC_RELOAD,
    TC_USE,
    TC_WEAPON_PREV,
    TC_WEAPON_NEXT,
    TC_ZOOM,
    TC_PAUSE,
    TC_COUNT
} TouchControl;

typedef struct TouchSlot {
    float cx;      /* center x, fraction of screen width  (0..1) */
    float cy;      /* center y, fraction of screen height (0..1) */
    float scale;   /* size multiplier, 1.0 = default size       */
    float opacity; /* 0..1                                       */
} TouchSlot;

typedef struct TouchLayout {
    TouchSlot slot[TC_COUNT];
} TouchLayout;

typedef struct TouchRect {
    float x, y, w, h; /* pixels, origin top-left */
} TouchRect;

#define TOUCH_SCALE_MIN   0.5f
#define TOUCH_SCALE_MAX   2.0f
#define TOUCH_OPACITY_MIN 0.10f
#define TOUCH_OPACITY_MAX 1.00f

/* Fill every slot with the default placement. */
void touchLayoutDefaults(TouchLayout *l);

/* Clamp one slot into its legal ranges (position kept on screen). */
void touchSlotClamp(TouchSlot *s);

/* Pixel rectangle for a control on a screen of sw x sh pixels. The control's
 * base size is baseW x baseH (the source PNG size). Its on-screen size scales
 * with the shorter screen side so it looks the same on every aspect ratio. */
TouchRect touchLayoutRect(const TouchLayout *l, TouchControl c,
                          float sw, float sh, float baseW, float baseH);

/* Which control, if any, is under the point (px, py)? Returns -1 for none.
 * Later controls win ties, so the one drawn on top is the one hit. */
int touchLayoutHit(const TouchLayout *l, float px, float py,
                   float sw, float sh, const float baseW[TC_COUNT],
                   const float baseH[TC_COUNT]);

/* Serialize to "key=value" lines. Returns bytes written (excluding the NUL),
 * or -1 if cap is too small. Keys look like "fire.cx", "fire.scale". */
int touchLayoutSerialize(const TouchLayout *l, char *buf, int cap);

/* Parse the same format. Starts from defaults, so missing keys keep their
 * default values; unknown keys and malformed lines are ignored. Values are
 * clamped. Always succeeds. */
void touchLayoutParse(TouchLayout *l, const char *buf);

/* Key prefix for a control, e.g. TC_FIRE -> "fire". */
const char *touchControlKey(TouchControl c);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_LAYOUT_H */
