#ifndef WAYLAND_CLIENT_H
#define WAYLAND_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

bool wayland_auto_connect();
bool wayland_is_connected();
void wayland_disconnect();

#ifdef __cplusplus
}
#endif

#endif
