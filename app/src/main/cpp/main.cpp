#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// دوال من theme.cpp (لا نعرّف struct Theme هنا)
extern "C" {
    extern void set_theme(int index);
    extern float get_bg_r();
    extern float get_bg_g();
    extern float get_bg_b();
}

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;

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
    // استخدام الدوال للحصول على لون الخلفية
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) init_egl(app->window);
            break;
        case APP_CMD_TERM_WINDOW:
            term_egl();
            break;
    }
}

void android_main(struct android_app* app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    set_theme(0); // Classic Black
    LOGI("android_main started with theme Classic Black");

    while (1) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
        }
        if (display) draw_frame();
    }
}
