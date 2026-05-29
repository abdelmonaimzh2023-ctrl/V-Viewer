#include <vector>
#include <algorithm>
#include <android_log.h>

#define TAG "V-Viewer-WinMgr"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ========== حالة النافذة ==========
enum WindowState {
    STATE_NORMAL,
    STATE_MINIMIZED,
    STATE_MAXIMIZED,
    STATE_CLOSED
};

// ========== هيكل النافذة ==========
struct Window {
    int id;
    float x, y;              // الموضع
    float width, height;     // الأبعاد
    float min_width, min_height;
    float z_order;           // الترتيب (من الأمام للخلف)
    WindowState state;
    bool focused;
    bool draggable;
    bool resizable;
    char title[128];
};

// ========== مدير النوافذ ==========
static std::vector<Window> windows;
static int next_id = 1;
static int focused_id = -1;
static float screen_width = 1920.0f;
static float screen_height = 1080.0f;

// ========== إنشاء نافذة ==========
int window_create(float x, float y, float w, float h, const char* title) {
    Window win;
    win.id = next_id++;
    win.x = x;
    win.y = y;
    win.width = w;
    win.height = h;
    win.min_width = 300.0f;
    win.min_height = 200.0f;
    win.z_order = (float)windows.size();
    win.state = STATE_NORMAL;
    win.focused = true;
    win.draggable = true;
    win.resizable = true;
    strncpy(win.title, title, sizeof(win.title) - 1);
    
    // إلغاء تركيز النوافذ الأخرى
    for (auto& w : windows) {
        w.focused = false;
    }
    
    windows.push_back(win);
    focused_id = win.id;
    
    LOGI("Window created: id=%d title=%s", win.id, title);
    return win.id;
}

// ========== إغلاق نافذة ==========
bool window_close(int id) {
    for (auto it = windows.begin(); it != windows.end(); ++it) {
        if (it->id == id) {
            it->state = STATE_CLOSED;
            LOGI("Window closed: id=%d", id);
            windows.erase(it);
            
            if (focused_id == id) {
                focused_id = windows.empty() ? -1 : windows.back().id;
                if (focused_id >= 0) {
                    window_focus(focused_id);
                }
            }
            return true;
        }
    }
    LOGE("Window not found: id=%d", id);
    return false;
}

// ========== تركيز نافذة ==========
bool window_focus(int id) {
    for (auto& win : windows) {
        if (win.id == id) {
            win.focused = true;
            win.z_order = windows.size() + 1.0f;
            win.state = (win.state == STATE_MINIMIZED) ? STATE_NORMAL : win.state;
            focused_id = id;
        } else {
            win.focused = false;
        }
    }
    return true;
}

// ========== تحريك نافذة ==========
bool window_move(int id, float x, float y) {
    for (auto& win : windows) {
        if (win.id == id && win.draggable) {
            // منع الخروج من الشاشة
            win.x = std::max(0.0f, std::min(x, screen_width - win.width));
            win.y = std::max(0.0f, std::min(y, screen_height - win.height));
            return true;
        }
    }
    return false;
}

// ========== تغيير حجم نافذة ==========
bool window_resize(int id, float w, float h) {
    for (auto& win : windows) {
        if (win.id == id && win.resizable && win.state == STATE_NORMAL) {
            win.width = std::max(win.min_width, std::min(w, screen_width - win.x));
            win.height = std::max(win.min_height, std::min(h, screen_height - win.y));
            return true;
        }
    }
    return false;
}

// ========== تصغير نافذة ==========
bool window_minimize(int id) {
    for (auto& win : windows) {
        if (win.id == id) {
            win.state = STATE_MINIMIZED;
            if (focused_id == id) {
                // تركيز النافذة التالية
                float max_z = -1.0f;
                int next_focus = -1;
                for (auto& w : windows) {
                    if (w.id != id && w.z_order > max_z && w.state != STATE_MINIMIZED) {
                        max_z = w.z_order;
                        next_focus = w.id;
                    }
                }
                focused_id = next_focus;
                if (next_focus >= 0) window_focus(next_focus);
            }
            return true;
        }
    }
    return false;
}

// ========== تكبير نافذة ==========
bool window_maximize(int id) {
    for (auto& win : windows) {
        if (win.id == id) {
            if (win.state == STATE_MAXIMIZED) {
                // إعادة للحجم الطبيعي
                win.state = STATE_NORMAL;
                win.x = 100.0f;
                win.y = 100.0f;
                win.width = 800.0f;
                win.height = 600.0f;
            } else {
                win.state = STATE_MAXIMIZED;
                win.x = 0.0f;
                win.y = 0.0f;
                win.width = screen_width;
                win.height = screen_height;
            }
            window_focus(id);
            return true;
        }
    }
    return false;
}

// ========== جلب نافذة للأمام ==========
bool window_bring_to_front(int id) {
    for (auto& win : windows) {
        if (win.id == id) {
            win.z_order = windows.size() + 1.0f;
            window_focus(id);
            return true;
        }
    }
    return false;
}

// ========== ترتيب النوافذ ==========
void window_sort_by_zorder() {
    std::sort(windows.begin(), windows.end(),
        [](const Window& a, const Window& b) {
            return a.z_order < b.z_order;
        });
}

// ========== الحصول على النوافذ المرئية ==========
std::vector<Window*> window_get_visible() {
    std::vector<Window*> visible;
    for (auto& win : windows) {
        if (win.state != STATE_MINIMIZED && win.state != STATE_CLOSED) {
            visible.push_back(&win);
        }
    }
    return visible;
}

// ========== الحصول على النوافذ المصغرة ==========
std::vector<Window*> window_get_minimized() {
    std::vector<Window*> minimized;
    for (auto& win : windows) {
        if (win.state == STATE_MINIMIZED) {
            minimized.push_back(&win);
        }
    }
    return minimized;
}

// ========== النافذة المركزة ==========
Window* window_get_focused() {
    for (auto& win : windows) {
        if (win.id == focused_id) return &win;
    }
    return nullptr;
}

// ========== نافذة عند الإحداثيات ==========
Window* window_at(float x, float y) {
    // من الأمام للخلف
    for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
        if (it->state == STATE_MINIMIZED || it->state == STATE_CLOSED) continue;
        if (x >= it->x && x <= it->x + it->width &&
            y >= it->y && y <= it->y + it->height) {
            return &(*it);
        }
    }
    return nullptr;
}

// ========== تعيين دقة الشاشة ==========
void window_set_screen_size(float w, float h) {
    screen_width = w;
    screen_height = h;
}

// ========== عدد النوافذ ==========
int window_count() {
    return (int)windows.size();
}

// ========== إغلاق الكل ==========
void window_close_all() {
    windows.clear();
    focused_id = -1;
    LOGI("All windows closed");
}
