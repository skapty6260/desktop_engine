#include <dbus-ipc/dbus-server.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>

struct dbus_server *dbus_server_create(void) {
    struct dbus_server *server = calloc(1, sizeof(struct dbus_server));
    if (server) {
        server->dbus_fd = -1;
    }
    return server;
}

struct dbus_wayland_integration_data *dbus_wayland_integration_create(struct wl_display *display) {
    struct dbus_wayland_integration_data *data = calloc(1, sizeof(struct dbus_wayland_integration_data));
    if (data) {
        data->wl_display = display;
    }
    return data;
}

bool dbus_server_init(struct dbus_server *server, struct dbus_wayland_integration_data *wayland_integration_data) {
    if (!wayland_integration_data || !server) return false;

    /* 1. Init D-Bus connection */
    if (!dbus_core_init_connection(server)) return false;

    /* 2. Integrate with wayland event loop */
    if (!dbus_wayland_integration_init(server, wayland_integration_data)) {
        dbus_core_cleanup(server);
        return false;
    }

    DBUS_INFO("D-Bus server fully initialized!");
    return true;
}