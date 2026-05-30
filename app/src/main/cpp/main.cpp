#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" void android_main(struct android_app* app) {
    LOGI("Minimal app started successfully");
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // خلفية خضراء زاهية
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
    // حلقة لا نهائية
    while (true) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) source->process(app, source);
        }
    }
}
