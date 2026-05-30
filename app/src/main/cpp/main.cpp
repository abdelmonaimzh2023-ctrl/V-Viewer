#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <chrono>
#include <sys/sysinfo.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// تعريف بسيط للـ Theme (forward declaration) – المحتوى الحقيقي في theme.cpp
struct Theme;

extern "C" {
    extern Theme* get_current_theme();
    extern void set_theme(int index);
    extern void auto_detect_quality();
    extern int get_current_quality();
}

static std::chrono::high_resolution_clock::time_point last_frame;
static float delta_time = 0.0f;
static float fps_timer = 0.0f;
static int frame_count = 0;
static int current_fps = 0;

static void render_frame() {
    auto now = std::chrono::high_resolution_clock::now();
    delta_time = std::chrono::duration<float>(now - last_frame).count();
    last_frame = now;
    frame_count++;
    fps_timer += delta_time;
    if (fps_timer >= 1.0f) {
        current_fps = frame_count;
        frame_count = 0;
        fps_timer = 0.0f;
        // تعديل الجودة تلقائياً حسب FPS – يمكن استدعاء دالة من theme.cpp لاحقاً
    }
    Theme* theme = get_current_theme();
    // الوصول لحقول bg_r... الخ موجودة في theme.cpp، هنا نستخدم المؤشر فقط
    // للتبسيط نضع ألوان افتراضية في حال عدم وجود الثيم
    glClearColor(0.05f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            last_frame = std::chrono::high_resolution_clock::now();
            auto_detect_quality();
            LOGI("V-Viewer initialized, quality: %d", get_current_quality());
            break;
        case APP_CMD_TERM_WINDOW:
            break;
    }
}

extern "C" void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    set_theme(0); // Classic Black
    LOGI("V-Viewer starting...");
    while (true) {
        int ident, events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) source->process(app, source);
        }
        render_frame();
    }
}
