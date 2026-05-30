#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <android/log.h>

#define TAG "V-Viewer-Wayland"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static int wayland_socket = -1;
static bool connected = false;
static char socket_path[256];

extern "C" {

bool wayland_connect(const char* path) {
    if (path == nullptr || strlen(path) < 1) {
        LOGE("Invalid socket path");
        return false;
    }
    strncpy(socket_path, path, sizeof(socket_path) - 1);
    wayland_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (wayland_socket < 0) {
        LOGE("Failed to create socket");
        return false;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
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

bool wayland_auto_connect() {
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

bool wayland_is_connected() {
    return connected && wayland_socket >= 0;
}

void wayland_disconnect() {
    if (wayland_socket >= 0) {
        close(wayland_socket);
        wayland_socket = -1;
    }
    connected = false;
    socket_path[0] = '\0';
    LOGI("Disconnected from Wayland");
}

} // extern "C"
