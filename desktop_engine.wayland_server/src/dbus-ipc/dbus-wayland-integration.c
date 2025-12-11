#include <dbus-ipc/dbus-wayland-integration.h>
#include <dbus-ipc/dbus-core.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>

int dbus_wayland_fd_callback(int fd, uint32_t mask, void *data) {
    struct dbus_server *server = (struct dbus_server *)data;

    if (!server || !server->connection) {
        DBUS_ERROR("fd_callback error: failed to get server from passed data");
        return 0;
    }

    DBUS_DEBUG("D-Bus fd callback triggered, mask: %u", mask);

    /* Handle D-Bus incoming messages */
    dbus_connection_read_write(server->connection, 0);
    while (dbus_connection_get_dispatch_status(server->connection) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(server->connection);
    }

    return 0;
}

bool dbus_wayland_integration_init(struct dbus_server *server, struct dbus_wayland_integration_data *integration_data) {
    if (!server || !server->initialized || !integration_data->wl_display) return false;

    /* Add D-Bus fd into wayland event loop */
    integration_data->wl_fd_source = wl_event_loop_add_fd(
        wl_display_get_event_loop(integration_data->wl_display),
        server->dbus_fd,
        WL_EVENT_READABLE,
        dbus_wayland_fd_callback,
        server
    );

    if (!integration_data->wl_fd_source) {
        DBUS_ERROR("Failed to add D-Bus fd to Wayland event loop");
        return false;
    }

    DBUS_INFO("D-Bus integrated with wayland event loop");
    return true;
}

void dbus_wayland_integration_cleanup(struct dbus_wayland_integration_data *integration_data) {
    if (!integration_data) return;

    /* Remove fd from wayland event loop */
    if (integration_data->wl_fd_source) {
        wl_event_source_remove(integration_data->wl_fd_source);
        integration_data->wl_fd_source = NULL;
    }

    integration_data->wl_display = NULL;
    free(integration_data);
}