#ifndef SERVER_H
#define SERVER_H

#include <wayland-server.h>

// Wayland server
// Should be compatible with xwayland in future

struct server {
    struct wl_display *display;
};

void server_init(struct server *server);
void server_run(struct server *server);
void server_cleanup(struct server *server);

#endif