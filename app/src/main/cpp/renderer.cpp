#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/log.h>
#include <string>
#include <cstdio>

#define TAG "V-Viewer-Renderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ========== وضع العرض ==========
enum DisplayMode {
    MODE_WAYLAND = 0,
    MODE_VNC     = 1
};

struct VNCConfig {
    std::string ip;
    int port;
    std::string password;
    bool connected;
};

// ========== حالة المحرك ==========
static DisplayMode current_mode = MODE_WAYLAND;
static VNCConfig vnc_config;
static bool initialized = false;
static std::string last_error = "";

// ========== تحميل Shader ==========
GLuint load_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        last_error = "Failed to create shader";
        LOGE("%s", last_error.c_str());
        return 0;
    }
    
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint info_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char* info_log = new char[info_len];
            glGetShaderInfoLog(shader, info_len, nullptr, info_log);
            last_error = std::string("Shader compile error: ") + info_log;
            LOGE("%s", last_error.c_str());
            delete[] info_log;
        }
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

// ========== تهيئة Wayland ==========
bool init_wayland() {
    LOGI("Initializing Wayland connection...");
    // هنا سيتم الاتصال بـ Wayland socket
    // سنضيف الكود في wayland_client.cpp
    current_mode = MODE_WAYLAND;
    return true;
}

// ========== تهيئة VNC ==========
bool init_vnc(const char* ip, int port, const char* password) {
    LOGI("Connecting to VNC: %s:%d", ip, port);
    
    vnc_config.ip = std::string(ip);
    vnc_config.port = port;
    vnc_config.password = std::string(password);
    vnc_config.connected = false;
    
    // محاولة اتصال وهمية للتجربة
    if (ip == nullptr || strlen(ip) < 1) {
        last_error = "Invalid IP address";
        LOGE("%s", last_error.c_str());
        return false;
    }
    
    if (port < 1 || port > 65535) {
        last_error = "Invalid port number";
        LOGE("%s", last_error.c_str());
        return false;
    }
    
    vnc_config.connected = true;
    current_mode = MODE_VNC;
    LOGI("VNC connected successfully");
    return true;
}

// ========== قطع اتصال VNC ==========
void disconnect_vnc() {
    vnc_config.connected = false;
    vnc_config.ip = "";
    vnc_config.port = 0;
    vnc_config.password = "";
    current_mode = MODE_WAYLAND;
    LOGI("VNC disconnected");
}

// ========== تهيئة المحرك ==========
bool init_renderer() {
    LOGI("Initializing renderer...");
    
    // إعدادات OpenGL الأساسية
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.05f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    initialized = true;
    last_error = "";
    LOGI("Renderer initialized successfully");
    return true;
}

// ========== رسم الإطار ==========
void render_frame() {
    if (!initialized) {
        if (!init_renderer()) return;
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    switch (current_mode) {
        case MODE_WAYLAND:
            // رسم النوافذ من Wayland
            // سيتم ربطه لاحقاً
            break;
        case MODE_VNC:
            // رسم إطار VNC
            // سيتم ربطه لاحقاً
            if (!vnc_config.connected) {
                LOGE("VNC not connected");
            }
            break;
    }
    
    // عرض أي أخطاء
    if (!last_error.empty()) {
        LOGI("Status: %s", last_error.c_str());
    }
}

// ========== الحصول على آخر خطأ ==========
const char* get_last_error() {
    return last_error.c_str();
}

// ========== الحصول على الوضع الحالي ==========
int get_display_mode() {
    return (int)current_mode;
}

// ========== هل VNC متصل ==========
bool is_vnc_connected() {
    return vnc_config.connected;
}

// ========== تنظيف ==========
void cleanup_renderer() {
    if (current_mode == MODE_VNC) {
        disconnect_vnc();
    }
    initialized = false;
    LOGI("Renderer cleaned up");
}
