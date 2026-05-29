#include <android_native_app_glue.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "V-Viewer", __VA_ARGS__))

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    LOGI("ANativeActivity_onCreate called");
    extern void android_main(struct android_app* app);
    struct android_app* app = (struct android_app*)malloc(sizeof(struct android_app));
    memset(app, 0, sizeof(struct android_app));
    app->activity = activity;
    activity->instance = app;
    android_main(app);
}
