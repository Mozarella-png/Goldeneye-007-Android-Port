/*
 * touchpad.c - Android touch controls: SDL virtual gamepad + GL overlay.
 * See port/include/touchpad.h. The mapping itself is in touch_map.c (pure,
 * host-tested); this file is the SDL and GL glue.
 */

/* Empty on PC builds (the root CMake globs every .c under port/src). The typedef
 * keeps the translation unit non-empty for -pedantic. */
typedef int ge_touchpad_pc_placeholder_t;

#if defined(__ANDROID__)

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include "glad/glad.h"

#include "platform.h"
#include "system.h"
#include "touch_layout.h"
#include "touch_map.h"
#include "touchpad.h"

static SDL_Joystick *s_joy = NULL;
static int           s_ready = 0;          /* virtual pad attached */
static TouchLayout   s_layout;
static TouchMapper   s_map;
static TouchVPad     s_pad;                /* last state pushed to SDL; read by the overlay */
static float         s_sw = 0.0f, s_sh = 0.0f;

/* GL overlay. s_gl: 0 = not tried, 1 = ready, -1 = unavailable. */
static int    s_gl = 0;
static GLuint s_prog = 0, s_vao = 0, s_vbo = 0;
static GLint  u_screen = -1, u_center = -1, u_radius = -1, u_color = -1;

/* Colours per control: rgb, and the alpha used while idle / pressed. */
static const float kColor[TC_COUNT][3] = {
    { 1.0f, 1.0f, 1.0f },  /* joystick */
    { 1.0f, 0.25f, 0.2f }, /* fire */
    { 1.0f, 0.9f, 0.2f },  /* reload */
    { 0.3f, 0.9f, 0.3f },  /* use */
    { 0.8f, 0.8f, 0.8f },  /* weapon prev */
    { 0.8f, 0.8f, 0.8f },  /* weapon next */
    { 0.3f, 0.6f, 1.0f },  /* zoom */
    { 0.8f, 0.8f, 0.8f },  /* pause */
};

static void pushPad(const TouchVPad *p)
{
    if (!s_joy) return;
    for (int i = 0; i < TM_AXIS_COUNT; i++)
        if (p->axis[i] != s_pad.axis[i])
            SDL_JoystickSetVirtualAxis(s_joy, i, p->axis[i]);
    for (int i = 0; i < TM_BUTTON_COUNT; i++)
        if (p->button[i] != s_pad.button[i])
            SDL_JoystickSetVirtualButton(s_joy, i, p->button[i]);
    s_pad = *p;
}

void touchpadInit(void)
{
    if (s_ready) return;

    /* A tap must not also act as the mouse: a mouse click would arm the
     * click-to-lock capture and move the game cursor. */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

    SDL_VirtualJoystickDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = TM_AXIS_COUNT;
    desc.nbuttons = TM_BUTTON_COUNT;
    /* SDL builds the standard controller mapping from these masks: the face
     * buttons, the shoulders, START/BACK, the stick clicks, the D-pad, and the
     * two sticks and two triggers. */
    desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_A) | (1u << SDL_CONTROLLER_BUTTON_B) |
                       (1u << SDL_CONTROLLER_BUTTON_X) | (1u << SDL_CONTROLLER_BUTTON_Y) |
                       (1u << SDL_CONTROLLER_BUTTON_BACK) | (1u << SDL_CONTROLLER_BUTTON_START) |
                       (1u << SDL_CONTROLLER_BUTTON_LEFTSTICK) | (1u << SDL_CONTROLLER_BUTTON_RIGHTSTICK) |
                       (1u << SDL_CONTROLLER_BUTTON_LEFTSHOULDER) | (1u << SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) |
                       (1u << SDL_CONTROLLER_BUTTON_DPAD_UP) | (1u << SDL_CONTROLLER_BUTTON_DPAD_DOWN) |
                       (1u << SDL_CONTROLLER_BUTTON_DPAD_LEFT) | (1u << SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_LEFTX) | (1u << SDL_CONTROLLER_AXIS_LEFTY) |
                     (1u << SDL_CONTROLLER_AXIS_RIGHTX) | (1u << SDL_CONTROLLER_AXIS_RIGHTY) |
                     (1u << SDL_CONTROLLER_AXIS_TRIGGERLEFT) | (1u << SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    desc.name = "GoldenEye touch controls";

    int idx = SDL_JoystickAttachVirtualEx(&desc);
    if (idx < 0) {
        sysLogPrintf(LOG_WARNING, "touch: virtual gamepad not attached: %s", SDL_GetError());
        return;
    }
    s_joy = SDL_JoystickOpen(idx);
    if (!s_joy) {
        sysLogPrintf(LOG_WARNING, "touch: could not open virtual gamepad %d: %s", idx, SDL_GetError());
        return;
    }

    touchLayoutDefaults(&s_layout);
    touchMapInit(&s_map, &s_layout, 1.0f, 1.0f);   /* real size arrives with the first event */
    s_ready = 1;
    sysLogPrintf(LOG_INFO, "touch: virtual gamepad attached (device %d)", idx);
}

void touchpadHandleEvent(const SDL_Event *ev)
{
    if (!s_ready) return;

    /* Touch coordinates are normalised; convert with the drawable size so they
     * line up with the overlay. Fall back to the last size drawn. */
    SDL_Window *w = SDL_GetWindowFromID(ev->tfinger.windowID);
    if (w) {
        int dw = 0, dh = 0;
        SDL_GL_GetDrawableSize(w, &dw, &dh);
        if (dw > 0 && dh > 0) { s_sw = (float)dw; s_sh = (float)dh; }
    }
    if (s_sw <= 0.0f || s_sh <= 0.0f) return;
    touchMapSetScreen(&s_map, s_sw, s_sh);

    long long id = (long long)ev->tfinger.fingerId;
    float px = ev->tfinger.x * s_sw;
    float py = ev->tfinger.y * s_sh;

    switch (ev->type) {
    case SDL_FINGERDOWN:   touchMapDown(&s_map, id, px, py); break;
    case SDL_FINGERMOTION: touchMapMove(&s_map, id, px, py); break;
    case SDL_FINGERUP:     touchMapUp(&s_map, id); break;
    default: return;
    }

    TouchVPad pad;
    touchMapState(&s_map, &pad);
    pushPad(&pad);
}

/* ---- GL overlay -------------------------------------------------------- */

static const char *kVS =
    "#version 300 es\n"
    "layout(location = 0) in vec2 aPos;\n"
    "uniform vec2 uScreen;\n"
    "void main() {\n"
    "    vec2 n = aPos / uScreen * 2.0 - 1.0;\n"
    "    gl_Position = vec4(n, 0.0, 1.0);\n"
    "}\n";

static const char *kFS =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec2 uCenter;\n"
    "uniform float uRadius;\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    float d = length(gl_FragCoord.xy - uCenter);\n"
    "    if (d > uRadius) discard;\n"
    "    float ring = step(uRadius - 3.0, d);\n"
    "    fragColor = vec4(uColor.rgb + ring * 0.3, uColor.a + ring * 0.25);\n"
    "}\n";

static GLuint compileShader(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = "";
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        sysLogPrintf(LOG_WARNING, "touch overlay: shader compile failed: %s", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static int glInitOverlay(void)
{
    if (!glCreateShader || !glCreateProgram || !glGenVertexArrays || !glGenBuffers)
        return 0;

    GLuint vs = compileShader(GL_VERTEX_SHADER, kVS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFS);
    if (!vs || !fs) return 0;

    s_prog = glCreateProgram();
    glAttachShader(s_prog, vs);
    glAttachShader(s_prog, fs);
    glLinkProgram(s_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = 0;
    glGetProgramiv(s_prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        sysLogPrintf(LOG_WARNING, "touch overlay: program link failed");
        return 0;
    }

    u_screen = glGetUniformLocation(s_prog, "uScreen");
    u_center = glGetUniformLocation(s_prog, "uCenter");
    u_radius = glGetUniformLocation(s_prog, "uRadius");
    u_color  = glGetUniformLocation(s_prog, "uColor");

    /* Own VAO + VBO so nothing the renderer bound is touched. */
    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);
    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return 1;
}

/* One filled disc, centre and radius in window pixels, origin bottom-left. */
static void drawDisc(float cx, float cy, float r, float red, float green, float blue, float alpha)
{
    float x0 = cx - r, y0 = cy - r, x1 = cx + r, y1 = cy + r;
    float v[8] = { x0, y0, x1, y0, x0, y1, x1, y1 };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glUniform2f(u_center, cx, cy);
    glUniform1f(u_radius, r);
    glUniform4f(u_color, red, green, blue, alpha);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/* Pressed state from the pad we last pushed. */
static int controlPressed(TouchControl c)
{
    switch (c) {
    case TC_FIRE:        return s_pad.axis[TM_AXIS_RIGHTTRIGGER] > 0;
    case TC_ZOOM:        return s_pad.axis[TM_AXIS_LEFTTRIGGER] > 0;
    case TC_RELOAD:      return s_pad.button[TM_BTN_X];
    case TC_USE:         return s_pad.button[TM_BTN_A];
    case TC_WEAPON_PREV: return s_pad.button[TM_BTN_DPAD_LEFT];
    case TC_WEAPON_NEXT: return s_pad.button[TM_BTN_Y];
    case TC_PAUSE:       return s_pad.button[TM_BTN_START];
    default:             return 0;
    }
}

void touchpadDraw(int w, int h)
{
    if (!s_ready || w <= 0 || h <= 0) return;
    s_sw = (float)w;
    s_sh = (float)h;
    if (s_gl == 0) s_gl = glInitOverlay() ? 1 : -1;
    if (s_gl < 0) return;

    /* Save only what this pass changes. */
    GLint prevProg = 0, prevVao = 0, prevVbo = 0, vp[4] = { 0, 0, 0, 0 };
    GLint srcRgb = 0, dstRgb = 0, srcA = 0, dstA = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_VIEWPORT, vp);
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcA);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &dstA);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean cull = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, w, h);

    glUseProgram(s_prog);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glUniform2f(u_screen, s_sw, s_sh);

    touchMapSetScreen(&s_map, s_sw, s_sh);

    /* Buttons and triggers: a circle per control, brighter while pressed. */
    for (int c = TC_FIRE; c < TC_COUNT; c++) {
        TouchRect r = touchLayoutRect(&s_layout, (TouchControl)c, s_sw, s_sh, 1.0f, 1.0f);
        float cx = r.x + r.w * 0.5f;
        float cy = (float)h - (r.y + r.h * 0.5f);
        int on = controlPressed((TouchControl)c);
        drawDisc(cx, cy, r.h * 0.5f,
                 kColor[c][0], kColor[c][1], kColor[c][2],
                 on ? 0.65f : 0.25f);
    }

    /* Left stick: the ring sits at the finger anchor while held, otherwise at
     * its fixed spot; the knob follows the thumb, clamped to the ring. */
    {
        float ringR = touchMapStickRadius(&s_map);
        float baseX, baseY;
        float knobX, knobY;
        int held = 0;
        for (int i = 0; i < TM_MAX_FINGERS && !held; i++) {
            const TouchFingerSlot *f = &s_map.f[i];
            if (!f->used || f->role != TM_ROLE_JOY) continue;
            baseX = f->ax; baseY = f->ay;
            knobX = f->px; knobY = f->py;
            held = 1;
        }
        if (!held) {
            TouchRect r = touchLayoutRect(&s_layout, TC_JOYSTICK, s_sw, s_sh, 1.0f, 1.0f);
            baseX = r.x + r.w * 0.5f;
            baseY = r.y + r.h * 0.5f;
            knobX = baseX; knobY = baseY;
        }
        float kx = knobX - baseX, ky = knobY - baseY;
        float len = kx * kx + ky * ky;
        if (len > ringR * ringR && len > 0.0f) {
            float s = ringR / sqrtf(len);   /* clamp the knob to the ring */
            kx *= s; ky *= s;
        }
        drawDisc(baseX, (float)h - baseY, ringR, 1.0f, 1.0f, 1.0f, held ? 0.30f : 0.15f);
        drawDisc(baseX + kx, (float)h - (baseY + ky), ringR * 0.35f, 1.0f, 1.0f, 1.0f, held ? 0.6f : 0.3f);
    }

    /* Restore. */
    glBindVertexArray((GLuint)prevVao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prevVbo);
    glUseProgram((GLuint)prevProg);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glBlendFuncSeparate((GLenum)srcRgb, (GLenum)dstRgb, (GLenum)srcA, (GLenum)dstA);
    if (depth)   glEnable(GL_DEPTH_TEST);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    if (cull)    glEnable(GL_CULL_FACE);
    if (!blend)  glDisable(GL_BLEND);
}

#endif /* __ANDROID__ */
