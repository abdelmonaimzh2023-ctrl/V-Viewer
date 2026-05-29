#include <android_native_app_glue.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <chrono>
#include <sys/sysinfo.h>

// ========== هيكل الثيم ==========
struct Theme {
    float bg_r, bg_g, bg_b;
    float accent_r, accent_g, accent_b;
    float corner_radius;
    float glow_intensity;
};

// ========== مستويات الجودة ==========
enum QualityLevel {
    QUALITY_ULTRA  = 0,  // 120 FPS - كل التأثيرات
    QUALITY_HIGH   = 1,  // 60 FPS  - تأثيرات كاملة
    QUALITY_MEDIUM = 2,  // 45 FPS  - تأثيرات مخفضة
    QUALITY_LOW    = 3,  // 30 FPS  - بدون ظلال
    QUALITY_MIN    = 4   // 15 FPS  - ألوان بسيطة فقط
};

struct QualitySettings {
    int target_fps;
    bool shadows;
    bool blur;
    bool glow;
    bool animations;
    bool rounded_corners;
    int color_depth;      // 24-bit عادي، 16-bit مخفض، 8-bit أدنى
};

QualitySettings quality_presets[] = {
    {120, true,  true,  true,  true,  true,  24},  // ULTRA
    {60,  true,  true,  true,  true,  true,  24},  // HIGH
    {45,  false, true,  true,  false, true,  16},  // MEDIUM
    {30,  false, false, false, false, true,  16},  // LOW
    {15,  false, false, false, false, false, 8}    // MIN
};

// ========== الثيمات العالمية ==========
Theme themes[] = {
    {0.05f, 0.0f, 0.1f,  0.4f, 0.0f, 1.0f, 16.0f, 1.5f},
    {0.02f, 0.02f, 0.05f, 0.0f, 1.0f, 0.8f, 14.0f, 1.8f},
    {0.08f, 0.0f, 0.0f,  1.0f, 0.2f, 0.4f, 18.0f, 1.3f},
    {0.0f, 0.05f, 0.0f,   0.0f, 1.0f, 0.3f, 12.0f, 2.0f}
};

int current_theme = 0;
Theme* active_theme = &themes[0];

int current_quality = QUALITY_HIGH;
QualitySettings* active_quality = &quality_presets[QUALITY_HIGH];

// ========== الوقت ==========
std::chrono::high_resolution_clock::time_point last_frame;
float delta_time = 0.0f;
float fps_timer = 0.0f;
int frame_count = 0;
int current_fps = 0;

// ========== كشف المعالج تلقائياً ==========
int detect_cpu_cores() {
    return get_nprocs_conf();
}

void auto_detect_quality() {
    int cores = detect_cpu_cores();
    if (cores >= 8)       set_quality(QUALITY_ULTRA);
    else if (cores >= 6)  set_quality(QUALITY_HIGH);
    else if (cores >= 4)  set_quality(QUALITY_MEDIUM);
    else if (cores >= 2)  set_quality(QUALITY_LOW);
    else                  set_quality(QUALITY_MIN);
}

// ========== تغيير الجودة ==========
void set_quality(int level) {
    if (level >= QUALITY_ULTRA && level <= QUALITY_MIN) {
        current_quality = level;
        active_quality = &quality_presets[level];
    }
}

// ========== نظام تقليل الألوان ==========
void apply_color_depth() {
    switch (active_quality->color_depth) {
        case 24: /* ألوان كاملة */ break;
        case 16: /* تقليل طفيف */ break;
        case 8:  /* ألوان أساسية فقط */ break;
    }
}

// ========== رسم الإطار ==========
static void render_frame() {
    auto now = std::chrono::high_resolution_clock::now();
    delta_time = std::chrono::duration<float>(now - last_frame).count();
    last_frame = now;
    
    // حساب FPS
    frame_count++;
    fps_timer += delta_time;
    if (fps_timer >= 1.0f) {
        current_fps = frame_count;
        frame_count = 0;
        fps_timer = 0.0f;
        
        // ضبط تلقائي إذا انخفض الأداء
        if (current_fps < active_quality->target_fps - 10) {
            if (current_quality < QUALITY_MIN) set_quality(current_quality + 1);
        }
    }
    
    // تطبيق عمق الألوان
    apply_color_depth();
    
    // خلفية داكنة
    glClearColor(active_theme->bg_r, active_theme->bg_g, active_theme->bg_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ========== الثيمات ==========
void set_theme(int index) {
    if (index >= 0 && index < 4) {
        current_theme = index;
        active_theme = &themes[index];
    }
}

// ========== الأوامر ==========
static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            last_frame = std::chrono::high_resolution_clock::now();
            auto_detect_quality();  // كشف تلقائي عند بدء التطبيق
            break;
        case APP_CMD_TERM_WINDOW:
            break;
    }
}

// ========== الدخول ==========
void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    set_theme(0);
    
    while (true) {
        android_poll_source* source = nullptr;
        int ident = ALooper_pollAll(0, nullptr, nullptr, (void**)&source);
        if (ident >= 0 && source != nullptr) {
            source->process(app, source);
        }
        render_frame();
    }
}
