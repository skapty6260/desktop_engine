#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "xdg-shell-protocol.h"
#include "logger/logger.h"
#include <wayland-server.h>

/* TODO
Create shm pool methods (To log client pixel buffers)
*/

static void bind_xdg_wm_base(struct wl_client *client, void *data,
                            uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &xdg_wm_base_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    xdg_wm_base_send_ping(resource, 1234); // Пример ping
    SERVER_DEBUG("XDG shell bound to client");
}

static void shm_create_pool(struct wl_client *client,
                           struct wl_resource *shm_resource,
                           uint32_t id, int fd, int32_t size) {
    SERVER_DEBUG("Creating SHM pool: fd=%d, size=%d", fd, size);
}

static const struct wl_shm_interface shm_implementation = {
    .create_pool = shm_create_pool,
};

static void bind_shm(struct wl_client *client, void *data,
                            uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_shm_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &shm_implementation, data, NULL);
    
    shm_send_formats(client, resource);
    
    SERVER_DEBUG("SHM bound to client");
}

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
    server->xdg_wm_base_global = wl_global_create(
        server->display,
        &xdg_wm_base_interface,
        1, server, bind_xdg_wm_base
    );

    server->shm_global = wl_global_create(
        server->display, 
        &wl_shm_interface, 
        1, server, bind_shm
    );

    if (!server->xdg_wm_base_global || !server->shm_global) {
        SERVER_FATAL("Failed to create Wayland globals");
    }

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