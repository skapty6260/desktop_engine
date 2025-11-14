#ifndef SERVER_H
#define SERVER_H

#include <wayland-server.h>

// Wayland server
// Should be compatible with xwayland in future

struct server {
    struct wl_display *display;
};

#endif