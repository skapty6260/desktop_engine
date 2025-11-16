#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>

void server_init(struct server *server) {
    SERVER_DEBUG("Initializing wayland server...");

    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }
}

void server_run(struct server *server) {
    SERVER_DEBUG("Running up wayland server...");
    wl_display_run(server->display);
}

// void server_cleanup(struct server *server) {}