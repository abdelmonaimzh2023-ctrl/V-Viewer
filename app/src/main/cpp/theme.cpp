#include <android/log.h>
#include <string>
#include <sys/sysinfo.h>

#define TAG "V-Viewer-Theme"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

struct Theme {
    std::string name;
    float bg_r, bg_g, bg_b;
    float accent_r, accent_g, accent_b;
    float text_r, text_g, text_b;
    float border_r, border_g, border_b;
    float corner_radius;
    float transparency;
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

static int current_quality = QUALITY_HIGH;
static QualitySettings* active_quality = &quality_presets[QUALITY_HIGH];

Theme themes[] = {
    {
        "Classic Black",
        0.08f, 0.08f, 0.10f,
        0.90f, 0.90f, 0.95f,
        0.85f, 0.85f, 0.90f,
        0.25f, 0.25f, 0.30f,
        6.0f, 0.95f
    },
    {
        "Pure White",
        0.95f, 0.95f, 0.97f,
        0.15f, 0.15f, 0.20f,
        0.10f, 0.10f, 0.15f,
        0.80f, 0.80f, 0.85f,
        4.0f, 0.98f
    },
    {
        "Royal Blue",
        0.02f, 0.05f, 0.15f,
        0.40f, 0.60f, 0.90f,
        0.75f, 0.80f, 0.90f,
        0.15f, 0.20f, 0.35f,
        5.0f, 0.92f
    },
    {
        "Professional Gray",
        0.15f, 0.15f, 0.18f,
        0.60f, 0.60f, 0.65f,
        0.80f, 0.80f, 0.85f,
        0.30f, 0.30f, 0.35f,
        4.0f, 0.90f
    },
    {
        "Olive Dark",
        0.05f, 0.08f, 0.05f,
        0.50f, 0.70f, 0.40f,
        0.70f, 0.80f, 0.65f,
        0.15f, 0.20f, 0.12f,
        5.0f, 0.93f
    },
    {
        "Luxury Brown",
        0.10f, 0.07f, 0.05f,
        0.75f, 0.55f, 0.35f,
        0.80f, 0.70f, 0.55f,
        0.25f, 0.18f, 0.12f,
        3.0f, 0.95f
    }
};

static int current_theme = 0;
static Theme* active_theme = &themes[0];

extern "C" {
    void set_quality(int level) {
        if (level >= QUALITY_ULTRA && level <= QUALITY_MIN) {
            current_quality = level;
            active_quality = &quality_presets[level];
        }
    }

    void auto_detect_quality() {
        int cores = get_nprocs_conf();
        if (cores >= 8)       set_quality(QUALITY_ULTRA);
        else if (cores >= 6)  set_quality(QUALITY_HIGH);
        else if (cores >= 4)  set_quality(QUALITY_MEDIUM);
        else if (cores >= 2)  set_quality(QUALITY_LOW);
        else                  set_quality(QUALITY_MIN);
        LOGI("Auto quality: %d cores -> level %d", cores, current_quality);
    }

    Theme* get_current_theme() {
        return active_theme;
    }

    bool set_theme(int index) {
        int count = sizeof(themes) / sizeof(themes[0]);
        if (index >= 0 && index < count) {
            current_theme = index;
            active_theme = &themes[index];
            LOGI("Theme set to: %s", active_theme->name.c_str());
            return true;
        }
        return false;
    }

    bool set_theme_by_name(const char* name) {
        int count = sizeof(themes) / sizeof(themes[0]);
        for (int i = 0; i < count; i++) {
            if (themes[i].name == std::string(name)) {
                return set_theme(i);
            }
        }
        return false;
    }

    const char* get_theme_name() {
        return active_theme->name.c_str();
    }

    int get_theme_count() {
        return sizeof(themes) / sizeof(themes[0]);
    }

    int get_current_quality() {
        return current_quality;
    }

    // دوال الألوان المباشرة
    float get_bg_r() { return active_theme->bg_r; }
    float get_bg_g() { return active_theme->bg_g; }
    float get_bg_b() { return active_theme->bg_b; }
}
