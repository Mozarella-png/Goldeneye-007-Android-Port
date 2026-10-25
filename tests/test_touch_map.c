/* Host tests for app/src/main/cpp/touch_map.c (no Android needed).
 * Build: gcc -std=c99 -Wall -Wextra -pedantic -Werror -Iapp/src/main/cpp \
 *          -o t tests/test_touch_map.c app/src/main/cpp/touch_map.c \
 *          app/src/main/cpp/touch_layout.c -lm && ./t */
#include <stdio.h>
#include <stdlib.h>
#include "touch_map.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); fails++; } } while (0)

/* Screen 1920x1080 (landscape phone). Layout defaults from touch_layout.c. */
#define SW 1920.0f
#define SH 1080.0f

static TouchMapper newMapper(void)
{
    TouchLayout l;
    TouchMapper m;
    touchLayoutDefaults(&l);
    touchMapInit(&m, &l, SW, SH);
    return m;
}

/* Centre of a control on screen, using the same base size the mapper uses. */
static void centreOf(const TouchLayout *l, TouchControl c, float *x, float *y)
{
    TouchRect r = touchLayoutRect(l, c, SW, SH, 1.0f, 1.0f);
    *x = r.x + r.w * 0.5f;
    *y = r.y + r.h * 0.5f;
}

int main(void)
{
    TouchVPad pad;
    TouchMapper m = newMapper();
    TouchLayout l;
    touchLayoutDefaults(&l);

    /* 1. Nothing touching: neutral pad. */
    touchMapState(&m, &pad);
    for (int i = 0; i < TM_AXIS_COUNT; i++) CHECK(pad.axis[i] == 0);
    for (int i = 0; i < TM_BUTTON_COUNT; i++) CHECK(pad.button[i] == 0);

    /* 2. Fire button -> right trigger full, nothing else. */
    {
        float x, y;
        centreOf(&l, TC_FIRE, &x, &y);
        touchMapDown(&m, 1, x, y);
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_RIGHTTRIGGER] == 32767);
        CHECK(pad.axis[TM_AXIS_LEFTTRIGGER] == 0);
        touchMapUp(&m, 1);
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_RIGHTTRIGGER] == 0);   /* released */
    }

    /* 3. Each button control maps to its virtual button, held only while down. */
    {
        struct { TouchControl c; int btn; } map[] = {
            { TC_USE, TM_BTN_A }, { TC_RELOAD, TM_BTN_X }, { TC_WEAPON_NEXT, TM_BTN_Y },
            { TC_WEAPON_PREV, TM_BTN_DPAD_LEFT }, { TC_PAUSE, TM_BTN_START },
        };
        for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
            float x, y;
            centreOf(&l, map[i].c, &x, &y);
            touchMapDown(&m, 10 + (long long)i, x, y);
            touchMapState(&m, &pad);
            CHECK(pad.button[map[i].btn] == 1);
            touchMapUp(&m, 10 + (long long)i);
            touchMapState(&m, &pad);
            CHECK(pad.button[map[i].btn] == 0);
        }
    }

    /* 4. Zoom (aim) -> left trigger full. */
    {
        float x, y;
        centreOf(&l, TC_ZOOM, &x, &y);
        touchMapDown(&m, 20, x, y);
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_LEFTTRIGGER] == 32767);
        touchMapUp(&m, 20);
    }

    /* 5. Joystick: fixed stick centred on its rect; drag right -> LEFTX max, LEFTY 0. */
    {
        float cx, cy;
        centreOf(&l, TC_JOYSTICK, &cx, &cy);
        float R = touchMapStickRadius(&m);
        touchMapDown(&m, 30, cx, cy);
        touchMapMove(&m, 30, cx + R, cy);           /* at full travel */
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_LEFTX] == 32767);
        CHECK(pad.axis[TM_AXIS_LEFTY] == 0);
        touchMapMove(&m, 30, cx, cy + R * 0.5f);    /* half, straight down */
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_LEFTX] == 0);
        CHECK(pad.axis[TM_AXIS_LEFTY] > 0);         /* SDL: down is positive */
        CHECK(pad.axis[TM_AXIS_LEFTY] < 32767);
        touchMapMove(&m, 30, cx, cy);               /* back to centre: deadzone */
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_LEFTX] == 0 && pad.axis[TM_AXIS_LEFTY] == 0);
        touchMapUp(&m, 30);
    }

    /* 6. Floating left stick: touch on the left half, outside any control. */
    {
        touchMapDown(&m, 40, 300.0f, 500.0f);
        touchMapMove(&m, 40, 300.0f - 10000.0f, 500.0f);   /* far left, clamps */
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_LEFTX] == -32767);
        touchMapUp(&m, 40);
    }

    /* 7. Look: right half, free area. Drag up -> RIGHTY negative. */
    {
        /* (1000, 900) is clear of every layout control at 1920x1080. */
        touchMapDown(&m, 50, 1000.0f, 900.0f);
        touchMapMove(&m, 50, 1000.0f, 900.0f - 10000.0f);
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_RIGHTY] == -32767);
        CHECK(pad.axis[TM_AXIS_RIGHTX] == 0);
        touchMapUp(&m, 50);
    }

    /* 8. Multi-touch: move with the left thumb while firing with the right. */
    {
        float fx, fy, cx, cy;
        centreOf(&l, TC_FIRE, &fx, &fy);
        centreOf(&l, TC_JOYSTICK, &cx, &cy);
        float R = touchMapStickRadius(&m);
        touchMapDown(&m, 60, fx, fy);                 /* fire */
        touchMapDown(&m, 61, cx, cy);                 /* joystick */
        touchMapMove(&m, 61, cx - R, cy);             /* stick left */
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_RIGHTTRIGGER] == 32767);
        CHECK(pad.axis[TM_AXIS_LEFTX] == -32767);
        touchMapUp(&m, 61);
        touchMapState(&m, &pad);
        CHECK(pad.axis[TM_AXIS_RIGHTTRIGGER] == 32767);   /* fire unaffected */
        CHECK(pad.axis[TM_AXIS_LEFTX] == 0);
        touchMapUp(&m, 60);
    }

    /* 9. Duplicate down for the same id acts as a move (no slot leak). */
    {
        touchMapDown(&m, 70, 1500.0f, 300.0f);
        touchMapDown(&m, 70, 1500.0f, 200.0f);
        int used = 0;
        for (int i = 0; i < TM_MAX_FINGERS; i++) used += m.f[i].used;
        CHECK(used == 1);
        touchMapUp(&m, 70);
    }

    /* 10. Overflow: more fingers than slots is ignored, not a crash. */
    {
        for (int i = 0; i < TM_MAX_FINGERS + 5; i++)
            touchMapDown(&m, 100 + i, 1500.0f, 100.0f + i);
        int used = 0;
        for (int i = 0; i < TM_MAX_FINGERS; i++) used += m.f[i].used;
        CHECK(used == TM_MAX_FINGERS);
        for (int i = 0; i < TM_MAX_FINGERS + 5; i++) touchMapUp(&m, 100 + i);
    }

    /* 11. Up for an unknown id is harmless. */
    touchMapUp(&m, 999);
    touchMapMove(&m, 999, 1.0f, 1.0f);

    if (fails) {
        printf("%d failure(s)\n", fails);
        return 1;
    }
    printf("touch_map: all checks passed\n");
    return 0;
}
