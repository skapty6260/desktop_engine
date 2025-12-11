#include <dbus-ipc/dbus-server.h>
#include <dbus-ipc/dbus-wayland-integration.h>
#include <dbus-ipc/dbus-core.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>

// Modules
#include <test-module.h>

struct dbus_server *dbus_server_create(void) {
    struct dbus_server *server = calloc(1, sizeof(struct dbus_server));
    if (server) {
        server->dbus_fd = -1;
        server->initialized = false;  // ИСПРАВЛЕНО: инициализируем как false
        server->connection = NULL;
        server->modules = NULL;
    }
    return server;
}

struct dbus_wayland_integration_data *dbus_wayland_integration_create(struct wl_display *display) {
    struct dbus_wayland_integration_data *data = calloc(1, sizeof(struct dbus_wayland_integration_data));
    if (data) {
        data->wl_display = display;
        data->wl_fd_source = NULL;
    }
    return data;
}

void dbus_server_cleanup(struct dbus_server *server, struct dbus_wayland_integration_data *wayland_integration_data) {
    /* Unregister modules */
    test_module_unregister(server);

    dbus_wayland_integration_cleanup(wayland_integration_data);
    dbus_core_cleanup(server);

    free(wayland_integration_data);
    free(server);
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

    /* 3. Register modules */
    if (!test_module_register(dbus_server)) {
        LOG_WARN(LOG_MODULE_CORE, "Failed to register test module");
    } else {
        LOG_INFO(LOG_MODULE_CORE, "Test module registered successfully");
        test_module_send_event(dbus_server, "startup", "Server started successfully");
    }

    return true;
}