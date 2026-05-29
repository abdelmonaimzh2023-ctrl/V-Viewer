#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <chrono>
#include <sys/sysinfo.h>

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

struct Theme {
    float bg_r, bg_g, bg_b;
    float accent_r, accent_g, accent_b;
    float corner_radius;
    float glow_intensity;
};

enum QualityLevel {
    QUALITY_ULTRA  = 0,
    QUALITY_HIGH   = 1,
    QUALITY_MEDIUM = 2,
    QUALITY_LOW    = 3,
    QUALITY_MIN    = 4
};

struct QualitySettings {
    int target_fps;
    bool shadows, blur, glow, animations, rounded_corners;
    int color_depth;
};

QualitySettings quality_presets[] = {
    {120, true,  true,  true,  true,  true,  24},
    {60,  true,  true,  true,  true,  true,  24},
    {45,  false, true,  true,  false, true,  16},
    {30,  false, false, false, false, true,  16},
    {15,  false, false, false, false, false, 8}
};

Theme themes[] = {
    {0.05f, 0.0f, 0.1f,  0.4f, 0.0f, 1.0f, 16.0f, 1.5f},
    {0.02f, 0.02f, 0.05f, 0.0f, 1.0f, 0.8f, 14.0f, 1.8f},
    {0.08f, 0.0f, 0.0f,  1.0f, 0.2f, 0.4f, 18.0f, 1.3f},
    {0.0f, 0.05f, 0.0f,   0.0f, 1.0f, 0.3f, 12.0f, 2.0f}
};

static int current_theme = 0;
static Theme* active_theme = &themes[0];
static int current_quality = QUALITY_HIGH;
static QualitySettings* active_quality = &quality_presets[QUALITY_HIGH];
static std::chrono::high_resolution_clock::time_point last_frame;
static float delta_time = 0.0f;
static float fps_timer = 0.0f;
static int frame_count = 0;
static int current_fps = 0;

int detect_cpu_cores() { return get_nprocs_conf(); }

void set_quality(int level) {
    if (level >= QUALITY_ULTRA && level <= QUALITY_MIN) {
        current_quality = level;
        active_quality = &quality_presets[level];
    }
}

void set_theme(int index) {
    if (index >= 0 && index < 4) {
        current_theme = index;
        active_theme = &themes[index];
    }
}

void auto_detect_quality() {
    int cores = detect_cpu_cores();
    if (cores >= 8)       set_quality(QUALITY_ULTRA);
    else if (cores >= 6)  set_quality(QUALITY_HIGH);
    else if (cores >= 4)  set_quality(QUALITY_MEDIUM);
    else if (cores >= 2)  set_quality(QUALITY_LOW);
    else                  set_quality(QUALITY_MIN);
}

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
        if (current_fps < active_quality->target_fps - 10) {
            if (current_quality < QUALITY_MIN) set_quality(current_quality + 1);
        }
    }
    glClearColor(active_theme->bg_r, active_theme->bg_g, active_theme->bg_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            last_frame = std::chrono::high_resolution_clock::now();
            auto_detect_quality();
            LOGI("V-Viewer initialized - Theme: %d, Quality: %d, FPS: %d", current_theme, current_quality, active_quality->target_fps);
            break;
        case APP_CMD_TERM_WINDOW:
            break;
    }
}

extern "C" void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    set_theme(0);
    LOGI("V-Viewer starting...");
    while (true) {
        int ident;
        int events;
        android_poll_source* source;
        while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) source->process(app, source);
        }
        render_frame();
    }
}
