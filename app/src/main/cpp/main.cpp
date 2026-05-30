#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" {
    extern void set_theme(int index);
    extern float get_bg_r();
    extern float get_bg_g();
    extern float get_bg_b();
    extern int get_theme_count();
}

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static int current_theme_index = 0;

// معالج اللمس – نقرة واحدة = تبديل الثيم
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            current_theme_index = (current_theme_index + 1) % get_theme_count();
            set_theme(current_theme_index);
            LOGI("Theme switched to %d", current_theme_index);
            return 1; // تم استهلاك الحدث
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
    LOGI("EGL initialized");
}

static void term_egl() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surface) eglDestroySurface(display, surface);
    if (context) eglDestroyContext(display, context);
    eglTerminate(display);
    display = EGL_NO_DISPLAY;
}

static void draw_frame() {
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                init_egl(app->window);
                app->onInputEvent = handle_input; // تفعيل اللمس
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
    set_theme(current_theme_index); // Classic Black أولاً
    LOGI("Tap screen to change theme");

    while (1) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
        }
        if (display) draw_frame();
    }
}
