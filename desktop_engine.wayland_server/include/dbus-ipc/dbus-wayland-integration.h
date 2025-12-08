#ifndef DBUS_WAYLAND_INTEGRATION_H
#define DBUS_WAYLAND_INTEGRATION_H

#include <wayland-server.h>
#include <dbus-ipc/dbus-core.h>

struct dbus_wayland_integration_data {
    struct wl_display *wl_display;
    struct wl_event_source *wl_fd_source;
};

bool dbus_wayland_integration_init(struct dbus_server *server, struct dbus_wayland_integration_data *integration_data);
void dbus_wayland_integration_cleanup(struct dbus_wayland_integration_data *integration_data);
int dbus_wayland_fd_callback(int fd, uint32_t mask, void *data);

#endif