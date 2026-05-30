#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>
#include <cstdlib>

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
static GLuint simple_program = 0;
static GLint u_color_loc = -1;

// شيدر بسيط جداً (vertex + fragment)
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
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    return shader;
}

static void init_shaders() {
    GLuint vs = load_shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fs = load_shader(GL_FRAGMENT_SHADER, fragment_src);
    simple_program = glCreateProgram();
    glAttachShader(simple_program, vs);
    glAttachShader(simple_program, fs);
    glLinkProgram(simple_program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    u_color_loc = glGetUniformLocation(simple_program, "uColor");
    LOGI("Shaders loaded, program=%u, uColor_loc=%d", simple_program, u_color_loc);
}

// معالج اللمس
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            current_theme_index = (current_theme_index + 1) % get_theme_count();
            set_theme(current_theme_index);
            LOGI("Theme switched to %d", current_theme_index);
            return 1;
        }
    }
    return 0;
}

static void init_egl(ANativeWindow* window) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    surface = eglCreateWindowSurface(display, config, window, NULL);
    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, NULL, ctxAttribs);
    eglMakeCurrent(display, surface, surface, context);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    init_shaders();
    LOGI("EGL initialized");
}

static void term_egl() {
    if (simple_program) glDeleteProgram(simple_program);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surface) eglDestroySurface(display, surface);
    if (context) eglDestroyContext(display, context);
    eglTerminate(display);
    display = EGL_NO_DISPLAY;
}

// رسم مستطيل باستخدام الشيدر
static void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    GLfloat verts[] = { x, y,  x+w, y,  x, y-h,  x+w, y-h };
    glUseProgram(simple_program);
    glUniform4f(u_color_loc, r, g, b, a);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
}

// رسم النافذة
static void draw_window() {
    float x = -0.6f, y = 0.7f, w = 1.2f, h = 1.0f;
    float border_thickness = 0.02f;

    // خلفية النافذة (داكنة شفافة قليلاً)
    draw_rect(x, y, w, h, 0.1f, 0.1f, 0.12f, 0.92f);

    // الإطار العلوي
    draw_rect(x, y + border_thickness, w, border_thickness, get_accent_r(), get_accent_g(), get_accent_b(), 1.0f);
    // الإطار السفلي
    draw_rect(x, y - h, w, border_thickness, get_accent_r(), get_accent_g(), get_accent_b(), 1.0f);
    // الإطار الأيسر
    draw_rect(x - border_thickness, y, border_thickness, h, get_accent_r(), get_accent_g(), get_accent_b(), 1.0f);
    // الإطار الأيمن
    draw_rect(x + w, y, border_thickness, h, get_accent_r(), get_accent_g(), get_accent_b(), 1.0f);
}

static void draw_frame() {
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_window();
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
    LOGI("Tap screen to change themes. Window frame should appear.");

    while (1) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
        }
        if (display && simple_program) draw_frame();
    }
}
