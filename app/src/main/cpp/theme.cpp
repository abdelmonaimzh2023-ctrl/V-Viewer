#include <android/log.h>
#include <string>

#define TAG "V-Viewer-Theme"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// ========== هيكل الثيم الكلاسيكي ==========
struct Theme {
    std::string name;
    
    // ألوان الخلفية
    float bg_r, bg_g, bg_b;
    
    // لون التمييز (للأزرار والأيقونات النشطة)
    float accent_r, accent_g, accent_b;
    
    // لون النص
    float text_r, text_g, text_b;
    
    // لون الإطارات
    float border_r, border_g, border_b;
    
    // الزوايا (كلاسيكية: حادة قليلاً)
    float corner_radius;
    
    // شفافية (زجاج خفيف)
    float transparency;
};

// ========== الثيمات الكلاسيكية ==========
Theme themes[] = {
    // 0 - أسود كلاسيكي
    {
        "Classic Black",
        0.08f, 0.08f, 0.10f,   // خلفية: أسود مزرق
        0.90f, 0.90f, 0.95f,   // تمييز: أبيض فضي
        0.85f, 0.85f, 0.90f,   // نص: أبيض ناعم
        0.25f, 0.25f, 0.30f,   // إطارات: رمادي داكن
        6.0f,                   // زوايا: حادة ناعمة
        0.95f                   // شفافية: خفيفة جداً
    },
    
    // 1 - أبيض نقي
    {
        "Pure White",
        0.95f, 0.95f, 0.97f,   // خلفية: أبيض دافئ
        0.15f, 0.15f, 0.20f,   // تمييز: أسود ناعم
        0.10f, 0.10f, 0.15f,   // نص: أسود فحمي
        0.80f, 0.80f, 0.85f,   // إطارات: رمادي فاتح
        4.0f,                   // زوايا: حادة
        0.98f                   // شفافية: شبه معتم
    },
    
    // 2 - أزرق غامق ملكي
    {
        "Royal Blue",
        0.02f, 0.05f, 0.15f,   // خلفية: أزرق غامق عميق
        0.40f, 0.60f, 0.90f,   // تمييز: أزرق ملكي
        0.75f, 0.80f, 0.90f,   // نص: أزرق فاتح
        0.15f, 0.20f, 0.35f,   // إطارات: أزرق داكن
        5.0f,                   // زوايا: متوسطة
        0.92f                   // شفافية
    },
    
    // 3 - رمادي احترافي
    {
        "Professional Gray",
        0.15f, 0.15f, 0.18f,   // خلفية: رمادي داكن
        0.60f, 0.60f, 0.65f,   // تمييز: رمادي متوسط
        0.80f, 0.80f, 0.85f,   // نص: رمادي فاتح
        0.30f, 0.30f, 0.35f,   // إطارات: رمادي
        4.0f,                   // زوايا: حادة
        0.90f                   // شفافية
    },
    
    // 4 - أخضر زيتوني
    {
        "Olive Dark",
        0.05f, 0.08f, 0.05f,   // خلفية: زيتوني غامق
        0.50f, 0.70f, 0.40f,   // تمييز: أخضر هادئ
        0.70f, 0.80f, 0.65f,   // نص: أخضر فاتح
        0.15f, 0.20f, 0.12f,   // إطارات: زيتوني
        5.0f,                   // زوايا
        0.93f                   // شفافية
    },
    
    // 5 - بني خشبي فاخر
    {
        "Luxury Brown",
        0.10f, 0.07f, 0.05f,   // خلفية: بني غامق
        0.75f, 0.55f, 0.35f,   // تمييز: ذهبي باهت
        0.80f, 0.70f, 0.55f,   // نص: كريمي
        0.25f, 0.18f, 0.12f,   // إطارات: بني داكن
        3.0f,                   // زوايا: حادة جداً
        0.95f                   // شفافية
    }
};

// ========== الحالة ==========
static int current_theme = 0;
static Theme* active_theme = &themes[0];

// ========== تغيير الثيم ==========
bool set_theme(int index) {
    int count = sizeof(themes) / sizeof(themes[0]);
    if (index >= 0 && index < count) {
        current_theme = index;
        active_theme = &themes[index];
        LOGI("Theme changed to: %s", active_theme->name.c_str());
        return true;
    }
    return false;
}

// ========== تغيير الثيم بالاسم ==========
bool set_theme_by_name(const char* name) {
    int count = sizeof(themes) / sizeof(themes[0]);
    for (int i = 0; i < count; i++) {
        if (themes[i].name == std::string(name)) {
            return set_theme(i);
        }
    }
    return false;
}

// ========== الحصول على الثيم الحالي ==========
Theme* get_current_theme() {
    return active_theme;
}

// ========== الحصول على اسم الثيم ==========
const char* get_theme_name() {
    return active_theme->name.c_str();
}

// ========== الحصول على عدد الثيمات ==========
int get_theme_count() {
    return sizeof(themes) / sizeof(themes[0]);
}

// ========== الحصول على أسماء كل الثيمات ==========
const char** get_all_theme_names() {
    static const char* names[6];
    for (int i = 0; i < 6; i++) {
        names[i] = themes[i].name.c_str();
    }
    return names;
}
