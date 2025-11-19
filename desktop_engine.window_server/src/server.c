// #define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>

static void handle_client_created(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, client_created_listener);
    struct wl_client *wl_client = data;

    SERVER_INFO("New client connected");
}

void server_init(struct server *server) {
    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Init clients list */
    wl_list_init(&server->clients);

    /* Create wayland globals */
    // server->compositor_global = wl_global_create(
    //     server->display,
    //     &wl_compositor_interface,
    //     4, server, bind_compositor
    // );

    // server->shell_global = wl_global_create(
    //     server->display,
    //     &wl_shell_interface,
    //     1, server, bind_compositor
    // );

    // server->shm_global = wl_global_create(
    //     server->display, 
    //     &wl_shm_interface, 
    //     1, server, bind_shm
    // );

    // if (!server->compositor_global || !server->shell_global || !server->shm_global) {
    //     SERVER_FATAL("Failed to create Wayland globals");
    // }

    /* Assign event listeners */
    server->client_created_listener.notify = handle_client_created;
    wl_display_add_client_created_listener(server->display, &server->client_created_listener);
}

void server_run(struct server *server) {
    SERVER_INFO("Wayland server is running");
    wl_display_run(server->display);
}

void server_cleanup(struct server *server) {
    if (server->display) {
        wl_display_destroy(server->display);
        server->display = NULL;
    }
}