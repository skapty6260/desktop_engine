#ifndef SERVER_H
#define SERVER_H

#include <wayland-server.h>

// Wayland server
// Should be compatible with xwayland in future
/* Что должен содержать в себе сервер
*/

struct server {
    struct wl_display *wl_display;
    struct wl_list client;
    struct wl_global *compositor_global;
    const char* socket;
};

struct client_state {
    struct wl_resource *resource;
    struct wl_link *link;
};

void server_init(struct server *server);
void server_run(struct server *server);
void server_cleanup(struct server *server);

#endif