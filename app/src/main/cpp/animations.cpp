#include <cmath>
#include <chrono>
#include <android_log.h>

#define TAG "V-Viewer-Anim"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// ========== منحنى الحركة (Easing) ==========
enum EaseType {
    EASE_LINEAR,
    EASE_OUT_QUAD,
    EASE_IN_OUT_CUBIC,
    EASE_OUT_BOUNCE,
    EASE_OUT_ELASTIC
};

// ========== قيمة متحركة ==========
struct AnimValue {
    float from;
    float to;
    float current;
    float duration;  // بالثواني
    float elapsed;
    bool playing;
    EaseType easing;
};

// ========== تطبيق منحنى الحركة ==========
float apply_easing(float t, EaseType type) {
    switch (type) {
        case EASE_LINEAR:
            return t;
        case EASE_OUT_QUAD:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case EASE_IN_OUT_CUBIC:
            if (t < 0.5f) return 4.0f * t * t * t;
            return 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        case EASE_OUT_BOUNCE:
            if (t < 1.0f / 2.75f) return 7.5625f * t * t;
            if (t < 2.0f / 2.75f) return 7.5625f * (t -= 1.5f / 2.75f) * t + 0.75f;
            if (t < 2.5f / 2.75f) return 7.5625f * (t -= 2.25f / 2.75f) * t + 0.9375f;
            return 7.5625f * (t -= 2.625f / 2.75f) * t + 0.984375f;
        case EASE_OUT_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return pow(2.0f, -10.0f * t) * sin((t - 0.075f) * (2.0f * M_PI) / 0.3f) + 1.0f;
        default:
            return t;
    }
}

// ========== بدء حركة ==========
void anim_start(AnimValue* anim, float from, float to, float duration, EaseType easing) {
    anim->from = from;
    anim->to = to;
    anim->current = from;
    anim->duration = duration;
    anim->elapsed = 0.0f;
    anim->playing = true;
    anim->easing = easing;
}

// ========== تحديث حركة ==========
void anim_update(AnimValue* anim, float delta_time) {
    if (!anim->playing) return;
    
    anim->elapsed += delta_time;
    float t = anim->elapsed / anim->duration;
    
    if (t >= 1.0f) {
        anim->current = anim->to;
        anim->playing = false;
        return;
    }
    
    float eased = apply_easing(t, anim->easing);
    anim->current = anim->from + (anim->to - anim->from) * eased;
}

// ========== إيقاف حركة ==========
void anim_stop(AnimValue* anim) {
    anim->playing = false;
}

// ========== إعادة تعيين ==========
void anim_reset(AnimValue* anim, float value) {
    anim->from = value;
    anim->to = value;
    anim->current = value;
    anim->elapsed = 0.0f;
    anim->playing = false;
}

// ========== فئة الحركات ==========
struct AnimationSystem {
    AnimValue window_scale;    // تكبير/تصغير النافذة
    AnimValue window_alpha;    // شفافية النافذة
    AnimValue window_x;        // موضع X
    AnimValue window_y;        // موضع Y
    AnimValue glow_pulse;      // نبض التوهج
    float time;
};

// ========== حركات جاهزة ==========

// فتح نافذة (تكبير + ظهور)
void anim_window_open(AnimationSystem* sys) {
    anim_start(&sys->window_scale, 0.8f, 1.0f, 0.3f, EASE_OUT_BOUNCE);
    anim_start(&sys->window_alpha, 0.0f, 1.0f, 0.25f, EASE_OUT_QUAD);
}

// إغلاق نافذة (تصغير + اختفاء)
void anim_window_close(AnimationSystem* sys) {
    anim_start(&sys->window_scale, 1.0f, 0.9f, 0.2f, EASE_IN_OUT_CUBIC);
    anim_start(&sys->window_alpha, 1.0f, 0.0f, 0.2f, EASE_OUT_QUAD);
}

// تصغير للتاسك بار
void anim_window_minimize(AnimationSystem* sys) {
    anim_start(&sys->window_scale, 1.0f, 0.3f, 0.4f, EASE_IN_OUT_CUBIC);
    anim_start(&sys->window_alpha, 1.0f, 0.5f, 0.3f, EASE_OUT_QUAD);
}

// تكبير كامل
void anim_window_maximize(AnimationSystem* sys) {
    anim_start(&sys->window_scale, 1.0f, 1.05f, 0.2f, EASE_OUT_ELASTIC);
}

// سحب نافذة (تتبع سلس)
void anim_window_drag(AnimationSystem* sys, float target_x, float target_y) {
    anim_start(&sys->window_x, sys->window_x.current, target_x, 0.15f, EASE_OUT_QUAD);
    anim_start(&sys->window_y, sys->window_y.current, target_y, 0.15f, EASE_OUT_QUAD);
}

// نبض توهج (للأيقونات النشطة)
void anim_glow_pulse(AnimationSystem* sys) {
    anim_start(&sys->glow_pulse, 0.6f, 1.0f, 1.5f, EASE_OUT_ELASTIC);
}

// ========== تهيئة النظام ==========
void anim_system_init(AnimationSystem* sys) {
    anim_reset(&sys->window_scale, 1.0f);
    anim_reset(&sys->window_alpha, 1.0f);
    anim_reset(&sys->window_x, 0.0f);
    anim_reset(&sys->window_y, 0.0f);
    anim_reset(&sys->glow_pulse, 0.8f);
    sys->time = 0.0f;
}

// ========== تحديث كل الحركات ==========
void anim_system_update(AnimationSystem* sys, float delta_time) {
    sys->time += delta_time;
    
    anim_update(&sys->window_scale, delta_time);
    anim_update(&sys->window_alpha, delta_time);
    anim_update(&sys->window_x, delta_time);
    anim_update(&sys->window_y, delta_time);
    anim_update(&sys->glow_pulse, delta_time);
    
    // إعادة نبض التوهج تلقائياً
    if (!sys->glow_pulse.playing && sys->glow_pulse.current > 0.9f) {
        anim_glow_pulse(sys);
    }
}
