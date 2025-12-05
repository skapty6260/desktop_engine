#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <wayland-server.h>

void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

void surface_headless_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y);

extern const struct wl_surface_interface surface_implementation;

#endif