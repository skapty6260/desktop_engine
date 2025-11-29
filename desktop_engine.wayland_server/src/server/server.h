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

struct buffer {
    uint32_t width, height;
    struct wl_resource *resource;

    enum wl_buffer_type {
        WL_BUFFER_SHM,
        WL_BUFFER_DMA_BUF,
        WL_BUFFER_EGL
    } type;

    union {
        struct {
            struct wl_shm_buffer *shm_buffer;
        } shm;
        struct {
            int fd;
            uint32_t format;
            uint64_t modifier;
        } dmabuf;
    };
};

struct surface {
    struct wl_resource *resource;
    struct wl_resource *xdg_surface;
    struct wl_resource *xdg_toplevel; 
    struct server *server;
    struct wl_list link;

    // Active state
    struct wl_resource *buffer;
    int width, height;
    int x,y;

    // Pending state
    struct wl_resource *pending_buffer;
    int pending_width, pending_height;
    int pending_x, pending_y;

    struct {
        bool attach : 1;
        bool damage : 1;
        bool transform : 1;
        bool scale : 1;
    } pending_changes;
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