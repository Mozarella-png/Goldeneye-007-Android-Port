#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "touch_layout.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); fails++; } } while (0)

static const float BW[TC_COUNT] = { 570, 852, 814, 498, 948, 948, 756, 494 };
static const float BH[TC_COUNT] = { 568, 1170, 680, 930, 800, 800, 540, 796 };

int main(void)
{
    TouchLayout l;
    touchLayoutDefaults(&l);

    /* defaults are on screen and in range */
    for (int i = 0; i < TC_COUNT; i++) {
        CHECK(l.slot[i].cx >= 0 && l.slot[i].cx <= 1);
        CHECK(l.slot[i].cy >= 0 && l.slot[i].cy <= 1);
        CHECK(l.slot[i].scale == 1.0f);
    }

    /* ultrawide 3440x1440 vs 1920x1080: same short side -> same control height */
    TouchRect a = touchLayoutRect(&l, TC_FIRE, 3440, 1440, BW[TC_FIRE], BH[TC_FIRE]);
    TouchRect b = touchLayoutRect(&l, TC_FIRE, 1920, 1080, BW[TC_FIRE], BH[TC_FIRE]);
    /* not stretched: aspect of rect equals aspect of asset */
    CHECK(a.w / a.h > BW[TC_FIRE] / BH[TC_FIRE] - 0.001f && a.w / a.h < BW[TC_FIRE] / BH[TC_FIRE] + 0.001f);
    CHECK(b.w / b.h > BW[TC_FIRE] / BH[TC_FIRE] - 0.001f && b.w / b.h < BW[TC_FIRE] / BH[TC_FIRE] + 0.001f);

    /* scale doubles size */
    l.slot[TC_FIRE].scale = 2.0f;
    TouchRect c = touchLayoutRect(&l, TC_FIRE, 1920, 1080, BW[TC_FIRE], BH[TC_FIRE]);
    CHECK(c.h > b.h * 1.99f && c.h < b.h * 2.01f);
    l.slot[TC_FIRE].scale = 1.0f;

    /* hit test: center of fire button hits fire */
    float cx = l.slot[TC_FIRE].cx * 1920, cy = l.slot[TC_FIRE].cy * 1080;
    CHECK(touchLayoutHit(&l, cx, cy, 1920, 1080, BW, BH) == TC_FIRE);
    /* far corner hits nothing */
    CHECK(touchLayoutHit(&l, 1, 1, 1920, 1080, BW, BH) == -1);

    /* clamp */
    TouchSlot s = { -3, 9, 10, 0 };
    touchSlotClamp(&s);
    CHECK(s.cx == 0 && s.cy == 1 && s.scale == TOUCH_SCALE_MAX && s.opacity == TOUCH_OPACITY_MIN);

    /* round trip */
    l.slot[TC_JOYSTICK].cx = 0.25f;
    l.slot[TC_JOYSTICK].opacity = 0.4f;
    l.slot[TC_PAUSE].scale = 1.5f;
    char buf[2048];
    int n = touchLayoutSerialize(&l, buf, sizeof buf);
    CHECK(n > 0);
    TouchLayout r;
    touchLayoutParse(&r, buf);
    CHECK(r.slot[TC_JOYSTICK].cx > 0.2499f && r.slot[TC_JOYSTICK].cx < 0.2501f);
    CHECK(r.slot[TC_JOYSTICK].opacity > 0.3999f && r.slot[TC_JOYSTICK].opacity < 0.4001f);
    CHECK(r.slot[TC_PAUSE].scale > 1.4999f && r.slot[TC_PAUSE].scale < 1.5001f);

    /* malformed input: unknown keys, garbage values, out-of-range values */
    const char *bad =
        "bogus.cx=0.5\n"
        "fire.cx=abc\n"
        "fire.scale=99\n"
        "reload.opacity=-5\n"
        "nonsense\n"
        "use.cy=0.33\n";
    TouchLayout m;
    touchLayoutParse(&m, bad);
    CHECK(m.slot[TC_FIRE].cx > 0.89f);
    CHECK(m.slot[TC_FIRE].scale == TOUCH_SCALE_MAX);
    CHECK(m.slot[TC_RELOAD].opacity == TOUCH_OPACITY_MIN);
    CHECK(m.slot[TC_USE].cy > 0.3299f && m.slot[TC_USE].cy < 0.3301f);

    /* serialize into a too-small buffer fails cleanly */
    char tiny[8];
    CHECK(touchLayoutSerialize(&l, tiny, sizeof tiny) == -1);

    /* parse NULL/empty gives defaults */
    TouchLayout e;
    touchLayoutParse(&e, "");
    CHECK(e.slot[TC_FIRE].cx == 0.90f || e.slot[TC_FIRE].cx > 0.89f);

    if (fails == 0) printf("touch_layout: all checks passed\n");
    return fails ? 1 : 0;
}
