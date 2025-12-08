#ifndef DBUS_SERVER_H
#define DBUS_SERVER_H

#include <dbus-ipc/dbus-wayland-integration.h>
#include <dbus-ipc/dbus-core.h>

struct dbus_server *dbus_server_create(void)
struct dbus_wayland_integration_data *dbus_wayland_integration_create(struct wl_display *display);

bool dbus_server_init(struct dbus_server *server, struct dbus_wayland_integration_data *wayland_integration_data);

#endif