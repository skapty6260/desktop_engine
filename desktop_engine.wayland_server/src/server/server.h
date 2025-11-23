#ifndef SERVER_H
#define SERVER_H

#include <wayland-server.h>

struct server {
    struct wl_display *display;
    const char* socket;

    struct wl_listener client_created_listener;

    struct wl_global *xdg_wm_base_global;
    struct wl_global *compositor_global;
    // struct wl_global *shell_global;
    struct wl_global *shm_global;

    struct wl_list clients;
    struct wl_list surfaces;
    struct wl_list shm_pools;
};

struct surface {
    struct wl_resource *resource;
    struct wl_resource *xdg_surface;
    struct wl_resource *xdg_toplevel; 
    struct server *server;
    struct wl_list link;
    // int width, height;
    struct wl_buffer *buffer;
};

struct client {
    struct wl_client *wl_client;
    struct wl_list link;
    struct wl_listener destroy_listener;
};

typedef struct server_config {
    char* startup_cmd;
} server_config_t;

void server_init(struct server *server);
void server_run(struct server *server);
void server_cleanup(struct server *server);

#endif