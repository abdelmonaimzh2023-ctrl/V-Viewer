#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// تعريف بسيط للثيم (موجود في theme.cpp)
struct Theme;
extern "C" {
    extern Theme* get_current_theme();
    extern void set_theme(int index);
    extern void auto_detect_quality();
}

// متغيرات الرسم
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
static EGLContext context = EGL_NO_CONTEXT;

// تهيئة EGL
static int init_display(struct android_app* app) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    
    surface = eglCreateWindowSurface(display, config, app->window, NULL);
    
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, NULL, contextAttribs);
    
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGE("eglMakeCurrent failed");
        return -1;
    }
    LOGI("EGL initialized");
    return 0;
}

// رسم إطار واحد
static void render_frame() {
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f); // أسود مزرق أنيق
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    eglSwapBuffers(display, surface);
}

// دالة الأوامر من الأندرويد
static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL) {
                init_display(app);
                auto_detect_quality();
                LOGI("Window ready, quality auto-detected");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            eglTerminate(display);
            display = EGL_NO_DISPLAY;
            break;
    }
}

extern "C" void android_main(struct android_app* app) {
    LOGI("V-Viewer started");
    app->onAppCmd = handle_cmd;
    set_theme(0); // Classic Black
    
    while (true) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) source->process(app, source);
        }
        // رسم فقط إذا كانت النافذة جاهزة
        if (display != EGL_NO_DISPLAY) {
            render_frame();
        }
    }
}
