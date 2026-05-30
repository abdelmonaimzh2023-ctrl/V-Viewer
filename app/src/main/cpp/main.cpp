#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>
#include <jni.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static int screen_w = 1920, screen_h = 1080;

static float to_ndc_x(float px) { return (px / screen_w) * 2.0f - 1.0f; }
static float to_ndc_y(float py) { return 1.0f - (py / screen_h) * 2.0f; }

static void openSettings(ANativeActivity* activity) {
    JNIEnv* env; activity->vm->AttachCurrentThread(&env, NULL);
    jclass clazz = env->GetObjectClass(activity->clazz);
    jmethodID method = env->GetMethodID(clazz, "openSettings", "()V");
    env->CallVoidMethod(activity->clazz, method);
    activity->vm->DetachCurrentThread();
}

static void openTerminal(ANativeActivity* activity) {
    JNIEnv* env; activity->vm->AttachCurrentThread(&env, NULL);
    jclass clazz = env->GetObjectClass(activity->clazz);
    jmethodID method = env->GetMethodID(clazz, "openTerminal", "()V");
    env->CallVoidMethod(activity->clazz, method);
    activity->vm->DetachCurrentThread();
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    float px = to_ndc_x(AMotionEvent_getX(event,0));
    float py = to_ndc_y(AMotionEvent_getY(event,0));
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    if (action == AMOTION_EVENT_ACTION_DOWN) {
        if (px > 0.5f && py > 0.5f) { openSettings(app->activity); return 1; }
        if (px < -0.5f && py > 0.5f) { openTerminal(app->activity); return 1; }
    }
    return 0;
}

static void init_egl(ANativeWindow* window) {
    screen_w = ANativeWindow_getWidth(window); screen_h = ANativeWindow_getHeight(window);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY); eglInitialize(display,0,0);
    const EGLint attribs[]={ EGL_SURFACE_TYPE,EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT, EGL_BLUE_SIZE,8, EGL_GREEN_SIZE,8, EGL_RED_SIZE,8, EGL_DEPTH_SIZE,16, EGL_NONE };
    EGLConfig cfg; EGLint n; eglChooseConfig(display,attribs,&cfg,1,&n);
    surface = eglCreateWindowSurface(display,cfg,window,0);
    const EGLint ctxAtt[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    context = eglCreateContext(display,cfg,NULL,ctxAtt);
    eglMakeCurrent(display,surface,surface,context);
}

static void term_egl() {
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    if(surface) eglDestroySurface(display,surface);
    if(context) eglDestroyContext(display,context);
    eglTerminate(display); display=EGL_NO_DISPLAY;
}

static void draw_frame() {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch(cmd) {
        case APP_CMD_INIT_WINDOW: if(app->window) { init_egl(app->window); app->onInputEvent = handle_input; } break;
        case APP_CMD_TERM_WINDOW: term_egl(); break;
    }
}

void android_main(struct android_app* app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    while(1) {
        int ident, events; android_poll_source* source;
        while((ident=ALooper_pollAll(0,NULL,&events,(void**)&source))>=0) { if(source) source->process(app,source); }
        if(display) draw_frame();
    }
}
