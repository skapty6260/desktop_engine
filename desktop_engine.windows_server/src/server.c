#define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>

void server_init(struct server *server) {
    SERVER_DEBUG("Initializing wayland server...");

    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Add socket. This should be last steps */
    server->socket = wl_display_add_socket_auto(server->display);
    if (!server->socket) {
        wl_display_destroy(server->display);
        SERVER_FATAL("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server->socket, true);

    SERVER_DEBUG("Added socket to wayland display");
    printf("Server socket: %s\n", server->socket);
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