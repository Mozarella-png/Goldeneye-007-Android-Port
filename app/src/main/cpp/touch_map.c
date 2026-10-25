#include "touch_map.h"

#include <math.h>
#include <string.h>

#define TM_STICK_DEADZONE   0.12f  /* fraction of the travel radius ignored */
#define TM_LOOK_FRACTION    0.12f  /* look full deflection: drag of 12% of the short side */

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float shortSide(const TouchMapper *m)
{
    return m->sw < m->sh ? m->sw : m->sh;
}

static int findSlot(const TouchMapper *m, long long id)
{
    for (int i = 0; i < TM_MAX_FINGERS; i++)
        if (m->f[i].used && m->f[i].id == id) return i;
    return -1;
}

void touchMapInit(TouchMapper *m, const TouchLayout *layout, float sw, float sh)
{
    memset(m, 0, sizeof(*m));
    m->layout = *layout;
    m->sw = sw;
    m->sh = sh;
}

void touchMapSetScreen(TouchMapper *m, float sw, float sh)
{
    m->sw = sw;
    m->sh = sh;
}

float touchMapStickRadius(const TouchMapper *m)
{
    /* Joystick rect height is the base control height; travel is 0.8 of it. */
    TouchRect r = touchLayoutRect(&m->layout, TC_JOYSTICK, m->sw, m->sh, 1.0f, 1.0f);
    return r.h * 0.8f;
}

void touchMapDown(TouchMapper *m, long long fingerId, float px, float py)
{
    if (findSlot(m, fingerId) >= 0) {
        touchMapMove(m, fingerId, px, py);
        return;
    }

    int slot = -1;
    for (int i = 0; i < TM_MAX_FINGERS; i++)
        if (!m->f[i].used) { slot = i; break; }
    if (slot < 0) return;  /* more fingers than we track: ignore the extra */

    TouchFingerSlot *f = &m->f[slot];
    memset(f, 0, sizeof(*f));
    f->used = 1;
    f->id = fingerId;
    f->px = f->ax = px;
    f->py = f->ay = py;

    float one[TC_COUNT];
    for (int i = 0; i < TC_COUNT; i++) one[i] = 1.0f;
    int hit = touchLayoutHit(&m->layout, px, py, m->sw, m->sh, one, one);

    if (hit == TC_JOYSTICK) {
        TouchRect r = touchLayoutRect(&m->layout, TC_JOYSTICK, m->sw, m->sh, 1.0f, 1.0f);
        f->role = TM_ROLE_JOY;
        f->ax = r.x + r.w * 0.5f;   /* a fixed stick is centred on its own rect */
        f->ay = r.y + r.h * 0.5f;
    } else if (hit >= 0) {
        f->role = TM_ROLE_CONTROL;
        f->ctl = (TouchControl)hit;
    } else if (px < m->sw * 0.5f) {
        f->role = TM_ROLE_JOY;      /* floating left stick */
    } else {
        f->role = TM_ROLE_LOOK;     /* right-side drag looks around */
    }
}

void touchMapMove(TouchMapper *m, long long fingerId, float px, float py)
{
    int i = findSlot(m, fingerId);
    if (i < 0) return;
    m->f[i].px = px;
    m->f[i].py = py;
}

void touchMapUp(TouchMapper *m, long long fingerId)
{
    int i = findSlot(m, fingerId);
    if (i < 0) return;
    memset(&m->f[i], 0, sizeof(m->f[i]));
}

/* Deflection of a stick from its anchor as signed 16-bit axes (x right, y down). */
static void stickVec(float dx, float dy, float radius, short *ox, short *oy)
{
    *ox = 0;
    *oy = 0;
    if (radius <= 0.0f) return;
    float nx = dx / radius, ny = dy / radius;
    float mag = sqrtf(nx * nx + ny * ny);
    if (mag < TM_STICK_DEADZONE) return;
    float v = clampf((mag - TM_STICK_DEADZONE) / (1.0f - TM_STICK_DEADZONE), 0.0f, 1.0f);
    float scale = v / mag;   /* keep the direction, set the magnitude */
    *ox = (short)lroundf(clampf(nx * scale, -1.0f, 1.0f) * 32767.0f);
    *oy = (short)lroundf(clampf(ny * scale, -1.0f, 1.0f) * 32767.0f);
}

void touchMapState(const TouchMapper *m, TouchVPad *out)
{
    memset(out, 0, sizeof(*out));

    float stickR = touchMapStickRadius(m);
    float lookR = shortSide(m) * TM_LOOK_FRACTION;

    /* Joystick and look: the first finger of each role drives it. */
    int haveJoy = 0, haveLook = 0;
    for (int i = 0; i < TM_MAX_FINGERS; i++) {
        const TouchFingerSlot *f = &m->f[i];
        if (!f->used) continue;
        if (f->role == TM_ROLE_JOY && !haveJoy) {
            stickVec(f->px - f->ax, f->py - f->ay, stickR,
                     &out->axis[TM_AXIS_LEFTX], &out->axis[TM_AXIS_LEFTY]);
            haveJoy = 1;
        } else if (f->role == TM_ROLE_LOOK && !haveLook) {
            stickVec(f->px - f->ax, f->py - f->ay, lookR,
                     &out->axis[TM_AXIS_RIGHTX], &out->axis[TM_AXIS_RIGHTY]);
            haveLook = 1;
        }
    }

    /* Layout controls: held while any finger is on them. */
    for (int i = 0; i < TM_MAX_FINGERS; i++) {
        const TouchFingerSlot *f = &m->f[i];
        if (!f->used || f->role != TM_ROLE_CONTROL) continue;
        switch (f->ctl) {
        case TC_FIRE:       out->axis[TM_AXIS_RIGHTTRIGGER] = 32767; break;
        case TC_ZOOM:       out->axis[TM_AXIS_LEFTTRIGGER]  = 32767; break;
        case TC_RELOAD:     out->button[TM_BTN_X] = 1; break;
        case TC_USE:        out->button[TM_BTN_A] = 1; break;
        case TC_WEAPON_PREV: out->button[TM_BTN_DPAD_LEFT] = 1; break;
        case TC_WEAPON_NEXT: out->button[TM_BTN_Y] = 1; break;
        case TC_PAUSE:      out->button[TM_BTN_START] = 1; break;
        default: break;
        }
    }
}
