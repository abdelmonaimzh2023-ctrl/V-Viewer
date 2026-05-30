#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>
#include <cstdlib>
#include <cmath>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" {
    extern void set_theme(int index);
    extern float get_bg_r(), get_bg_g(), get_bg_b();
    extern float get_accent_r(), get_accent_g(), get_accent_b();
    extern float get_border_r(), get_border_g(), get_border_b();
    extern int get_theme_count();
}

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static int current_theme_index = 0;
static GLuint program = 0;
static GLint uColorLoc = -1;

// أبعاد الشاشة والنافذة
static int screen_w = 1920, screen_h = 1080;
static float win_x = 0.2f, win_y = 0.8f, win_w = 0.6f, win_h = 0.7f; // إحداثيات نسبية
static bool maximized = false;
static float prev_x, prev_y, prev_w, prev_h; // لحفظ الحالة قبل التكبير
static float title_bar_h = 0.06f; // ارتفاع شريط العنوان (نسبي للشاشة)

// أزرار النافذة (دائرية)
struct Button { float x, y, r; };
static Button btn_close, btn_max, btn_min;

// موقع اللمس
static float touch_x = -1, touch_y = -1;
static bool dragging = false;
static float drag_off_x = 0, drag_off_y = 0;

// تحويل الإحداثيات من البكسل إلى النظام النسبي (-1..1)
static float to_ndc_x(float px) { return (px / screen_w) * 2.0f - 1.0f; }
static float to_ndc_y(float py) { return 1.0f - (py / screen_h) * 2.0f; }
static float ndc_to_px_w(float w) { return w * screen_w / 2.0f; }
static float ndc_to_px_h(float h) { return h * screen_h / 2.0f; }

// حساب أزرار شريط العنوان (داخل النافذة)
static void update_buttons() {
    float btn_radius = title_bar_h * 0.35f;
    float btn_y = win_y - title_bar_h / 2.0f;
    // من اليمين إلى اليسار: إغلاق (أحمر), تكبير (أخضر), تصغير (أصفر)
    float start_x = win_x + win_w - btn_radius * 2.0f;
    btn_close = { start_x, btn_y, btn_radius };
    start_x -= btn_radius * 3.5f;
    btn_max   = { start_x, btn_y, btn_radius };
    start_x -= btn_radius * 3.5f;
    btn_min   = { start_x, btn_y, btn_radius };
}

// الشيدر
const char* vertex_src = R"(#version 310 es
precision highp float;
layout(location = 0) in vec2 pos;
void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
})";

const char* fragment_src = R"(#version 310 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
})";

static GLuint load_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

static void init_shaders() {
    GLuint vs = load_shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fs = load_shader(GL_FRAGMENT_SHADER, fragment_src);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    uColorLoc = glGetUniformLocation(program, "uColor");
}

// رسم مستطيل
static void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    float verts[] = { x, y,  x+w, y,  x, y-h,  x+w, y-h };
    glUseProgram(program);
    glUniform4f(uColorLoc, r, g, b, a);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
}

// رسم دائرة
static void draw_circle(float cx, float cy, float r, float red, float green, float blue, float alpha) {
    const int segments = 32;
    float verts[(segments+2)*2];
    verts[0] = cx; verts[1] = cy;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        verts[2 + i*2] = cx + cosf(angle) * r;
        verts[2 + i*2 + 1] = cy + sinf(angle) * r;
    }
    glUseProgram(program);
    glUniform4f(uColorLoc, red, green, blue, alpha);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments+2);
    glDisableVertexAttribArray(0);
}

// فحص إذا كانت النقطة داخل دائرة
static bool in_circle(float px, float py, const Button& b) {
    float dx = px - b.x, dy = py - b.y;
    return (dx*dx + dy*dy) <= b.r*b.r;
}

// فحص إذا كانت النقطة داخل مستطيل (عنوان النافذة)
static bool in_title_bar(float px, float py) {
    return (px >= win_x && px <= win_x + win_w && py >= win_y - title_bar_h && py <= win_y);
}

// معالج اللمس
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        float px = to_ndc_x(AMotionEvent_getX(event, 0));
        float py = to_ndc_y(AMotionEvent_getY(event, 0));
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            // فحص الأزرار أولاً
            if (in_circle(px, py, btn_close)) {
                // إغلاق (حالياً لا شيء، سنضيفه لاحقاً)
                LOGI("Close button pressed");
                return 1;
            }
            if (in_circle(px, py, btn_max)) {
                // تكبير / استعادة
                if (maximized) {
                    // استعادة
                    win_x = prev_x; win_y = prev_y; win_w = prev_w; win_h = prev_h;
                    maximized = false;
                } else {
                    // تكبير
                    prev_x = win_x; prev_y = win_y; prev_w = win_w; prev_h = win_h;
                    win_x = -0.98f; win_y = 0.98f; win_w = 1.96f; win_h = 1.88f - title_bar_h;
                    maximized = true;
                }
                update_buttons();
                LOGI("Maximize toggled");
                return 1;
            }
            if (in_circle(px, py, btn_min)) {
                // تصغير (مؤقتاً يخفي النافذة)
                LOGI("Minimize pressed (not implemented)");
                return 1;
            }
            // بدء السحب إذا كان الضغط على شريط العنوان
            if (in_title_bar(px, py)) {
                dragging = true;
                drag_off_x = win_x - px;
                drag_off_y = win_y - py;
                return 1;
            }
            // وإلا تغيير الثيم
            current_theme_index = (current_theme_index + 1) % get_theme_count();
            set_theme(current_theme_index);
            LOGI("Theme switched to %d", current_theme_index);
            return 1;
        }
        else if (action == AMOTION_EVENT_ACTION_MOVE && dragging) {
            win_x = px + drag_off_x;
            win_y = py + drag_off_y;
            update_buttons();
            return 1;
        }
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            dragging = false;
            return 1;
        }
    }
    return 0;
}

static void init_egl(ANativeWindow* window) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);
    const EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE };
    EGLConfig config; EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    surface = eglCreateWindowSurface(display, config, window, NULL);
    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, NULL, ctxAttribs);
    eglMakeCurrent(display, surface, surface, context);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    init_shaders();
    update_buttons();
    LOGI("EGL initialized");
}

static void term_egl() {
    if (program) glDeleteProgram(program);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surface) eglDestroySurface(display, surface);
    if (context) eglDestroyContext(display, context);
    eglTerminate(display);
    display = EGL_NO_DISPLAY;
}

static void draw_frame() {
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // توهج نيون بسيط (مستطيل ضبابي خلف الإطار)
    float glow_r = get_accent_r(), glow_g = get_accent_g(), glow_b = get_accent_b();
    float glow_size = 0.015f;
    draw_rect(win_x - glow_size, win_y + glow_size, win_w + glow_size*2, glow_size, glow_r, glow_g, glow_b, 0.4f);
    draw_rect(win_x - glow_size, win_y - win_h, win_w + glow_size*2, glow_size, glow_r, glow_g, glow_b, 0.4f);
    draw_rect(win_x - glow_size, win_y, glow_size, win_h, glow_r, glow_g, glow_b, 0.4f);
    draw_rect(win_x + win_w, win_y, glow_size, win_h, glow_r, glow_g, glow_b, 0.4f);

    // شريط العنوان (خلفية النافذة + عنوان)
    draw_rect(win_x, win_y, win_w, title_bar_h, get_accent_r(), get_accent_g(), get_accent_b(), 1.0f);
    // جسم النافذة (داكن مع شفافية بسيطة)
    draw_rect(win_x, win_y - title_bar_h, win_w, win_h - title_bar_h, 0.1f, 0.1f, 0.12f, 0.92f);

    // أزرار (دائرية)
    draw_circle(btn_close.x, btn_close.y, btn_close.r, 0.95f, 0.3f, 0.3f, 1.0f);
    draw_circle(btn_max.x, btn_max.y, btn_max.r, 0.3f, 0.9f, 0.3f, 1.0f);
    draw_circle(btn_min.x, btn_min.y, btn_min.r, 0.95f, 0.8f, 0.2f, 1.0f);

    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                init_egl(app->window);
                app->onInputEvent = handle_input;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            term_egl();
            break;
    }
}

void android_main(struct android_app* app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    set_theme(current_theme_index);
    LOGI("V-Viewer ready with elegant window UI");

    while (1) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
        }
        if (display && program) draw_frame();
    }
}
