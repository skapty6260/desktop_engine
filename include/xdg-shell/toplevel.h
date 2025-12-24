#ifndef XDG_SHELL_TOPLEVEL_H
#define XDG_SHELL_TOPLEVEL_H

#include <stdint.h>
#include <wayland-server.h>

void xdg_toplevel_destroy(struct wl_client *client, struct wl_resource *resource);
void xdg_toplevel_set_title(struct wl_client *client, struct wl_resource *resource, const char *title);
void xdg_toplevel_set_app_id(struct wl_client *client, struct wl_resource *resource, const char *app_id);

extern const struct xdg_toplevel_interface xdg_toplevel_implementation;

#endif