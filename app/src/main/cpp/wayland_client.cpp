#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <android/log.h>

#define TAG "V-Viewer-Wayland"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static int wayland_socket = -1;
static bool connected = false;

extern "C" {

bool wayland_connect_tcp(const char* ip, int port) {
    wayland_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (wayland_socket < 0) {
        LOGE("Failed to create TCP socket");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(ip, &addr.sin_addr);

    if (connect(wayland_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("Failed to connect to %s:%d", ip, port);
        close(wayland_socket);
        wayland_socket = -1;
        return false;
    }

    connected = true;
    LOGI("Connected to Wayland TCP %s:%d", ip, port);
    return true;
}

bool wayland_auto_connect() {
    // نحاول الاتصال عبر TCP على المنفذ 12345
    return wayland_connect_tcp("127.0.0.1", 12345);
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
    LOGI("Disconnected from Wayland TCP");
}

} // extern "C"
