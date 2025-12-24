#ifndef SERVER_H
#define SERVER_H

#include <wayland-server.h>
#include <dbus-server/server.h>

struct server {
    struct wl_display *display;
    const char* socket;

    struct wl_global *xdg_wm_base_global;
    struct wl_global *compositor_global;
    struct wl_global *shm_global;

    struct wl_list surfaces;
    struct wl_list shm_pools;

    struct dbus_server *dbus_server;
};

struct surface {
    struct wl_resource *resource;
    struct wl_resource *xdg_surface;
    struct wl_resource *xdg_toplevel; 
    struct server *server;
    struct wl_list link;
};

typedef struct server_config {
    char* startup_cmd;
} server_config_t;

void server_init(struct server *server);
void server_run(struct server *server);
void server_cleanup(struct server *server);

void server_set_dbus(struct server *server, struct dbus_server *dbus_server);

#endif