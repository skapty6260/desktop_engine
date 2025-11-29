#include <server/wayland/compositor.h>
#include <server/server.h>
#include <logger/logger.h>
#include <stdlib.h>

static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);
    
    SERVER_DEBUG("SURFACE: Resource destroyed, surface=%p", surface);
    
    if (surface) {
        wl_list_remove(&surface->link);
        free(surface);
    }
}

static void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    SERVER_DEBUG("SURFACE DAMAGE CALLED");
}

static void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
    SERVER_DEBUG("SURFACE FRAME CALLED");
}

static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    SERVER_DEBUG("SURFACE SET_OPAQUE_REGION CALLED");
}

static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    SERVER_DEBUG("SURFACE SET_INPUT_REGION CALLED");
}

static void surface_commit(struct wl_client *client, struct wl_resource *resource) {
    // struct surface *surface = wl_resource_get_user_data(resource);

    // if (!surface) return;
    
    // if (surface->pending_changes.attach == true) {
    //     if (surface->buffer) {
    //         wl_buffer_send_release(surface->buffer);
    //     }

    //     // Set new buffer (May be NULL to detach)
    //     if (surface->pending_buffer) {
    //         surface->buffer = surface->pending_buffer;
    //         surface->pending_buffer = NULL; // Передача владения

    //         // Обновляем размеры поверхности (Should add get buffer size)
    //         int width, height;
    //         // if (get_buffer_size(surface->buffer, &width, &height)) {
    //         //     // Применяем масштабирование
    //         //     surface->width = width / surface->buffer_scale;
    //         //     surface->height = height / surface->buffer_scale;
    //         // }
    //         SERVER_DEBUG("Surface committed: buffer=%p, size=%dx%d", surface->buffer, surface->width, surface->height);
    //     } else {
    //         // Buffer detach
    //         surface->width = 0;
    //         surface->height = 0;
    //         SERVER_DEBUG("Surface committed: buffer detached");
    //     }

    //     surface->pending_changes.attach = false;
    // }
    SERVER_DEBUG("SURFACE COMMIT CALLED");
}

static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform) {
    SERVER_DEBUG("SURFACE SET_BUFFER_TRANSFORM CALLED");
}

static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale) {
    SERVER_DEBUG("SURFACE SET_BUFFER_SCALE CALLED");
}

static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    SERVER_DEBUG("SURFACE DAMAGE BUFFER CALLED");
}

static void surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {
    SERVER_DEBUG("SURFACE OFFSET CALLED");
}

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