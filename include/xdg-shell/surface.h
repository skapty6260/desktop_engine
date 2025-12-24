#ifndef XDG_SHELL_SURFACE_H
#define XDG_SHELL_SURFACE_H

#include <stdint.h>
#include <wayland-server.h>
#include <wayland/server.h>

void xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource);
void xdg_surface_get_toplevel(struct wl_client *client, struct wl_resource *resource, uint32_t id);
void xdg_surface_get_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *parent, struct wl_resource *positioner);
void xdg_surface_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
void xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial);

extern const struct xdg_surface_interface xdg_surface_implementation;

#endif