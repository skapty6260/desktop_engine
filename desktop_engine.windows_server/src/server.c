#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>
#include <stdlib.h>

void server_init(struct server *server) {
    SERVER_DEBUG("Initializing wayland server...");

    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Add socket. This should be last steps */
    server->socket = wl_display_add_socket_auto(server->wl_display);
    if (!server->socket) {
        wl_display_destroy(server->display);
        SERVER_FATAL("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server->socket, true);

    SERVER_DEBUG("Added socket to wayland display");
}

void server_run(struct server *server) {
    SERVER_INFO("Wayland server is running");
    wl_display_run(server->display);
}

void server_cleanup(struct server *server) {
    if (server->display) {
        wl_display_destroy(server->display);
    }
}