#include <xdg-shell/toplevel.h>
#include <logger.h>

#include "xdg-shell-protocol.h"

void xdg_toplevel_destroy(struct wl_client *client, struct wl_resource *resource) {
    wl_resource_destroy(resource);
}

void xdg_toplevel_set_title(struct wl_client *client, struct wl_resource *resource, const char *title) {
    SERVER_DEBUG("XDG toplevel title set: %s", title);
}

void xdg_toplevel_set_app_id(struct wl_client *client, struct wl_resource *resource, const char *app_id) {
    SERVER_DEBUG("XDG toplevel app_id set: %s", app_id);
}

const struct xdg_toplevel_interface xdg_toplevel_implementation = {
    .destroy = xdg_toplevel_destroy,
    .set_title = xdg_toplevel_set_title,
    .set_app_id = xdg_toplevel_set_app_id,
    // Остальные методы можно оставить NULL
};