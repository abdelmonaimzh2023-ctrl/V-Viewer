#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>
#include <cmath>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" {
    extern void set_theme(int index);
    extern float get_bg_r(), get_bg_g(), get_bg_b();
    extern float get_accent_r(), get_accent_g(), get_accent_b();
}

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static GLuint program = 0;
static GLint uColorLoc = -1;

static int screen_w = 1920, screen_h = 1080;

static float win_x = 0.2f, win_y = 0.8f, win_w = 0.6f, win_h = 0.7f;
static bool maximized = false;
static bool minimized = false;
static float prev_x, prev_y, prev_w, prev_h;
static float title_h = 0.06f;

struct Button { float x, y, r; };
static Button btnClose, btnMax, btnMin;

static bool dragging = false;
static float drag_off_x, drag_off_y;

static void update_buttons() {
    float r = title_h * 0.35f;
    float by = win_y - title_h / 2.0f;
    float start_x = win_x + win_w - r * 1.5f;
    btnClose = { start_x, by, r };
    start_x -= r * 3.5f;
    btnMax   = { start_x, by, r };
    start_x -= r * 3.5f;
    btnMin   = { start_x, by, r };
}

static float to_ndc_x(float px) { return (px / screen_w) * 2.0f - 1.0f; }
static float to_ndc_y(float py) { return 1.0f - (py / screen_h) * 2.0f; }

const char* vs_src = R"(#version 310 es
precision highp float;
layout(location=0) in vec2 pos;
void main() { gl_Position = vec4(pos,0,1); })";

const char* fs_src = R"(#version 310 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; })";

static GLuint load_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

static void init_shaders() {
    GLuint vs = load_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = load_shader(GL_FRAGMENT_SHADER, fs_src);
    program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    uColorLoc = glGetUniformLocation(program, "uColor");
}

// رسم خط (شريحة رفيعة)
static void draw_line(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.0001f) return;
    float nx = -dy / len * thickness * 0.5f;
    float ny =  dx / len * thickness * 0.5f;
    float verts[] = {
        x1 - nx, y1 - ny,
        x2 - nx, y2 - ny,
        x1 + nx, y1 + ny,
        x2 - nx, y2 - ny,
        x2 + nx, y2 + ny,
        x1 + nx, y1 + ny
    };
    glUseProgram(program);
    glUniform4f(uColorLoc, r, g, b, a);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
}

// رسم مستطيل
static void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    float verts[] = { x,y, x+w,y, x,y-h, x+w,y-h };
    glUseProgram(program);
    glUniform4f(uColorLoc, r,g,b,a);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glDisableVertexAttribArray(0);
}

// أيقونة X
static void draw_close_icon(float cx, float cy, float size, float r, float g, float b) {
    float half = size * 0.5f;
    draw_line(cx - half, cy + half, cx + half, cy - half, size*0.15f, r, g, b, 1.0f);
    draw_line(cx - half, cy - half, cx + half, cy + half, size*0.15f, r, g, b, 1.0f);
}

// أيقونة تكبير (مربع)
static void draw_max_icon(float cx, float cy, float size, float r, float g, float b) {
    float half = size * 0.4f;
    draw_rect(cx - half, cy + half, half*2, half*2, r, g, b, 1.0f);
    // إطار مربع (رسم خطوط)
    float t = size * 0.1f;
    draw_line(cx - half, cy + half, cx + half, cy + half, t, r, g, b, 1.0f);
    draw_line(cx + half, cy + half, cx + half, cy - half, t, r, g, b, 1.0f);
    draw_line(cx + half, cy - half, cx - half, cy - half, t, r, g, b, 1.0f);
    draw_line(cx - half, cy - half, cx - half, cy + half, t, r, g, b, 1.0f);
}

// أيقونة تصغير (شرطة)
static void draw_min_icon(float cx, float cy, float size, float r, float g, float b) {
    float half = size * 0.4f;
    draw_line(cx - half, cy, cx + half, cy, size*0.15f, r, g, b, 1.0f);
}

static bool in_circle(float px, float py, const Button& b) {
    float dx=px-b.x, dy=py-b.y;
    return (dx*dx+dy*dy) <= b.r*b.r;
}

static bool in_title(float px, float py) {
    return (px>=win_x && px<=win_x+win_w && py>=win_y-title_h && py<=win_y);
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    float px = to_ndc_x(AMotionEvent_getX(event,0));
    float py = to_ndc_y(AMotionEvent_getY(event,0));
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

    if (action == AMOTION_EVENT_ACTION_DOWN) {
        if (minimized) { minimized = false; update_buttons(); return 1; }
        if (in_circle(px,py, btnClose)) { LOGI("Close"); return 1; }
        if (in_circle(px,py, btnMax)) {
            if (maximized) { win_x=prev_x; win_y=prev_y; win_w=prev_w; win_h=prev_h; maximized=false; }
            else { prev_x=win_x; prev_y=prev_y; prev_w=win_w; prev_h=win_h; win_x=-0.98f; win_y=0.98f; win_w=1.96f; win_h=1.88f; maximized=true; }
            update_buttons(); return 1;
        }
        if (in_circle(px,py, btnMin)) { minimized = true; return 1; }
        if (in_title(px,py)) { dragging = true; drag_off_x = win_x - px; drag_off_y = win_y - py; return 1; }
        return 0;
    }
    else if (action == AMOTION_EVENT_ACTION_MOVE && dragging) {
        win_x = px + drag_off_x; win_y = py + drag_off_y; update_buttons(); return 1;
    }
    else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        dragging = false; return 1;
    }
    return 0;
}

static void init_egl(ANativeWindow* window) {
    screen_w = ANativeWindow_getWidth(window);
    screen_h = ANativeWindow_getHeight(window);
    LOGI("Screen: %dx%d", screen_w, screen_h);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display,0,0);
    const EGLint attribs[]={ EGL_SURFACE_TYPE,EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
                              EGL_BLUE_SIZE,8, EGL_GREEN_SIZE,8, EGL_RED_SIZE,8, EGL_DEPTH_SIZE,16, EGL_NONE };
    EGLConfig cfg; EGLint n;
    eglChooseConfig(display,attribs,&cfg,1,&n);
    surface = eglCreateWindowSurface(display,cfg,window,0);
    const EGLint ctxAtt[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    context = eglCreateContext(display,cfg,NULL,ctxAtt);
    eglMakeCurrent(display,surface,surface,context);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    init_shaders();
    update_buttons();
}

static void term_egl() {
    if(program) glDeleteProgram(program);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    if(surface) eglDestroySurface(display,surface);
    if(context) eglDestroyContext(display,context);
    eglTerminate(display);
    display=EGL_NO_DISPLAY;
}

static void draw_frame() {
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!minimized) {
        float gx = get_accent_r(), gy = get_accent_g(), gz = get_accent_b();
        float s = 0.012f;
        draw_rect(win_x-s, win_y+s, win_w+s*2, s, gx,gy,gz,0.4f);
        draw_rect(win_x-s, win_y-win_h, win_w+s*2, s, gx,gy,gz,0.4f);
        draw_rect(win_x-s, win_y, s, win_h, gx,gy,gz,0.4f);
        draw_rect(win_x+win_w, win_y, s, win_h, gx,gy,gz,0.4f);
        draw_rect(win_x, win_y, win_w, title_h, gx, gy, gz, 1.0f);
        draw_rect(win_x, win_y-title_h, win_w, win_h-title_h, 0.1f,0.1f,0.12f,0.92f);

        // أيقونات بدلاً من الدوائر
        float icon_size = btnClose.r * 1.8f;
        draw_close_icon(btnClose.x, btnClose.y, icon_size, 1.0f, 1.0f, 1.0f);
        draw_max_icon(btnMax.x, btnMax.y, icon_size, 1.0f, 1.0f, 1.0f);
        draw_min_icon(btnMin.x, btnMin.y, icon_size, 1.0f, 1.0f, 1.0f);
    }
    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch(cmd) {
        case APP_CMD_INIT_WINDOW:
            if(app->window) { init_egl(app->window); app->onInputEvent = handle_input; }
            break;
        case APP_CMD_TERM_WINDOW: term_egl(); break;
    }
}

void android_main(struct android_app* app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    set_theme(0);
    while(1) {
        int ident, events;
        android_poll_source* src;
        while((ident=ALooper_pollAll(0,NULL,&events,(void**)&src))>=0) {
            if(src) src->process(app,src);
        }
        if(display && program) draw_frame();
    }
}
