#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <android/log.h>

#define TAG "V-Viewer-Wayland"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ========== حالة الاتصال ==========
static int wayland_socket = -1;
static bool connected = false;
static char socket_path[256];

// ========== الاتصال بمقبس Wayland ==========
bool wayland_connect(const char* path) {
    if (path == nullptr || strlen(path) < 1) {
        LOGE("Invalid socket path");
        return false;
    }
    
    // حفظ المسار
    strncpy(socket_path, path, sizeof(socket_path) - 1);
    
    // إنشاء مقبس Unix
    wayland_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (wayland_socket < 0) {
        LOGE("Failed to create socket");
        return false;
    }
    
    // إعداد العنوان
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    
    // اتصال
    if (connect(wayland_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("Failed to connect to Wayland at %s", path);
        close(wayland_socket);
        wayland_socket = -1;
        return false;
    }
    
    connected = true;
    LOGI("Connected to Wayland: %s", path);
    return true;
}

// ========== إرسال بيانات ==========
bool wayland_send(const void* data, size_t size) {
    if (!connected || wayland_socket < 0) {
        LOGE("Not connected to Wayland");
        return false;
    }
    
    ssize_t sent = send(wayland_socket, data, size, 0);
    if (sent < 0) {
        LOGE("Failed to send data");
        return false;
    }
    
    return true;
}

// ========== استقبال بيانات ==========
int wayland_receive(void* buffer, size_t size) {
    if (!connected || wayland_socket < 0) {
        LOGE("Not connected to Wayland");
        return -1;
    }
    
    ssize_t received = recv(wayland_socket, buffer, size, 0);
    if (received < 0) {
        LOGE("Failed to receive data");
        return -1;
    }
    
    return (int)received;
}

// ========== فحص حالة الاتصال ==========
bool wayland_is_connected() {
    return connected && wayland_socket >= 0;
}

// ========== الحصول على مسار المقبس ==========
const char* wayland_get_path() {
    return socket_path;
}

// ========== محاولة اتصال تلقائي ==========
bool wayland_auto_connect() {
    // قائمة المسارات المحتملة لمقبس Wayland
    const char* paths[] = {
        "/tmp/runtime-dir/wayland-1",
        "/tmp/runtime-dir/wayland-0",
        "/data/data/com.termux/files/usr/tmp/runtime-dir/wayland-1",
        "/data/data/com.termux/files/usr/tmp/runtime-dir/wayland-0",
        nullptr
    };
    
    for (int i = 0; paths[i] != nullptr; i++) {
        if (wayland_connect(paths[i])) {
            LOGI("Auto-connected to: %s", paths[i]);
            return true;
        }
    }
    
    LOGE("Could not find Wayland socket");
    return false;
}

// ========== قطع الاتصال ==========
void wayland_disconnect() {
    if (wayland_socket >= 0) {
        close(wayland_socket);
        wayland_socket = -1;
    }
    connected = false;
    socket_path[0] = '\0';
    LOGI("Disconnected from Wayland");
}
