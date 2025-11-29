#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <wayland-server.h>

void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

const struct wl_surface_interface surface_implementation = {
    .destroy = surface_destroy,
    .attach = surface_headless_attach,
    .damage = surface_damage,
    .frame = surface_frame,
    .set_opaque_region = surface_set_opaque_region,
    .set_input_region = surface_set_input_region,
    .commit = surface_commit,
    .set_buffer_transform = surface_set_buffer_transform,
    .set_buffer_scale = surface_set_buffer_scale,
    .damage_buffer = surface_damage_buffer,
    .offset = surface_offset,
};

void surface_destroy(struct wl_client *client, struct wl_resource *resource);
void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
void surface_commit(struct wl_client *client, struct wl_resource *resource);
void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform);
void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale);
void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
void surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y);
void surface_headless_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y);

#endif