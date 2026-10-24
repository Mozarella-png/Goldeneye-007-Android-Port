#include "touch_layout.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *const kKeys[TC_COUNT] = {
    "joystick", "fire", "reload", "use", "weaponprev", "weaponnext", "zoom", "pause"
};

/* Default centers, following the reference layout (joystick bottom-left,
 * action buttons bottom-right, pause top-right). */
static const float kDefCx[TC_COUNT] = { 0.12f, 0.90f, 0.78f, 0.66f, 0.80f, 0.92f, 0.70f, 0.95f };
static const float kDefCy[TC_COUNT] = { 0.72f, 0.78f, 0.52f, 0.40f, 0.20f, 0.20f, 0.62f, 0.08f };

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

const char *touchControlKey(TouchControl c)
{
    if ((int)c < 0 || (int)c >= TC_COUNT) return "";
    return kKeys[c];
}

void touchSlotClamp(TouchSlot *s)
{
    s->cx = clampf(s->cx, 0.0f, 1.0f);
    s->cy = clampf(s->cy, 0.0f, 1.0f);
    s->scale = clampf(s->scale, TOUCH_SCALE_MIN, TOUCH_SCALE_MAX);
    s->opacity = clampf(s->opacity, TOUCH_OPACITY_MIN, TOUCH_OPACITY_MAX);
}

void touchLayoutDefaults(TouchLayout *l)
{
    for (int i = 0; i < TC_COUNT; i++) {
        l->slot[i].cx = kDefCx[i];
        l->slot[i].cy = kDefCy[i];
        l->slot[i].scale = 1.0f;
        l->slot[i].opacity = 0.7f;
    }
}

/* Size is relative to the shorter screen side so controls keep their
 * proportions on ultrawide and tall phones alike. */
#define TOUCH_BASE_FRACTION  0.16f /* control height as fraction of short side */

TouchRect touchLayoutRect(const TouchLayout *l, TouchControl c,
                          float sw, float sh, float baseW, float baseH)
{
    TouchRect r = { 0, 0, 0, 0 };
    if ((int)c < 0 || (int)c >= TC_COUNT || sw <= 0.0f || sh <= 0.0f || baseH <= 0.0f)
        return r;
    const TouchSlot *s = &l->slot[c];
    float shortSide = sw < sh ? sw : sh;
    float hPx = shortSide * TOUCH_BASE_FRACTION * s->scale;
    float wPx = hPx * (baseW / baseH);
    r.w = wPx;
    r.h = hPx;
    r.x = s->cx * sw - wPx * 0.5f;
    r.y = s->cy * sh - hPx * 0.5f;
    return r;
}

int touchLayoutHit(const TouchLayout *l, float px, float py,
                   float sw, float sh, const float baseW[TC_COUNT],
                   const float baseH[TC_COUNT])
{
    int hit = -1;
    for (int i = 0; i < TC_COUNT; i++) {
        TouchRect r = touchLayoutRect(l, (TouchControl)i, sw, sh, baseW[i], baseH[i]);
        if (px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h)
            hit = i;
    }
    return hit;
}

int touchLayoutSerialize(const TouchLayout *l, char *buf, int cap)
{
    int total = 0;
    for (int i = 0; i < TC_COUNT; i++) {
        char line[160];
        int n = snprintf(line, sizeof line,
                         "%s.cx=%.4f\n%s.cy=%.4f\n%s.scale=%.4f\n%s.opacity=%.4f\n",
                         kKeys[i], l->slot[i].cx, kKeys[i], l->slot[i].cy,
                         kKeys[i], l->slot[i].scale, kKeys[i], l->slot[i].opacity);
        if (n < 0 || total + n >= cap) return -1;
        memcpy(buf + total, line, (size_t)n);
        total += n;
    }
    if (total < cap) buf[total] = '\0';
    return total;
}

static int findControl(const char *name, size_t len)
{
    for (int i = 0; i < TC_COUNT; i++) {
        if (strlen(kKeys[i]) == len && strncmp(kKeys[i], name, len) == 0)
            return i;
    }
    return -1;
}

void touchLayoutParse(TouchLayout *l, const char *buf)
{
    touchLayoutDefaults(l);
    if (!buf) return;

    const char *p = buf;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        const char *eq = memchr(p, '=', len);
        const char *dot = memchr(p, '.', len);
        if (eq && dot && dot < eq) {
            int c = findControl(p, (size_t)(dot - p));
            if (c >= 0) {
                const char *field = dot + 1;
                size_t flen = (size_t)(eq - field);
                char *end = NULL;
                float v = strtof(eq + 1, &end);
                if (end != eq + 1) {
                    TouchSlot *s = &l->slot[c];
                    if (flen == 2 && strncmp(field, "cx", 2) == 0) s->cx = v;
                    else if (flen == 2 && strncmp(field, "cy", 2) == 0) s->cy = v;
                    else if (flen == 5 && strncmp(field, "scale", 5) == 0) s->scale = v;
                    else if (flen == 7 && strncmp(field, "opacity", 7) == 0) s->opacity = v;
                }
            }
        }
        if (!eol) break;
        p = eol + 1;
    }
    for (int i = 0; i < TC_COUNT; i++) touchSlotClamp(&l->slot[i]);
}
