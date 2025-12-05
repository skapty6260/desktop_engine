#include <wayland/compositor.h>
#include <wayland/buffer.h>
#include <logger.h>
#include <wayland/server.h>
#include <stdlib.h>

#ifdef HAVE_LINUX_DMABUF
#include "linux-dmabuf-unstable-v1-protocol.h"
#endif

// SURFACES
static struct buffer *custom_buffer_from_resource(struct wl_resource *resource) {
    struct buffer *buffer = calloc(1, sizeof(struct buffer));
    if (!buffer) {
        SERVER_ERROR("Failed to allocate memory for buffer");
        return NULL;
    }
    
    struct buffer *custom_shm_buffer = wl_resource_get_user_data(resource);
    if (custom_shm_buffer) {
        // Это наш ручной SHM буфер
        buffer->type = WL_BUFFER_SHM;
        buffer->resource = resource;
        buffer->width = custom_shm_buffer->width;
        buffer->height = custom_shm_buffer->height;
        
        return buffer;
    }

#ifdef HAVE_LINUX_DMABUF
    // Проверяем DMA-BUF через linux-dmabuf extension
    if (wl_resource_instance_of(resource, &zwp_linux_buffer_params_v1_interface, NULL)) {
        buffer->type = WL_BUFFER_DMA_BUF;
        // Получаем параметры DMA-BUF
        SERVER_DEBUG("Buffer is dmabuf!");
        return buffer;
    }
#endif

    // Если не SHM и не DMA-BUF, освобождаем память
    free(buffer);
    return NULL;
}

static const char* buffer_type_to_string(struct buffer *buffer) {
    switch (buffer->type) {
        case WL_BUFFER_SHM:
        return "SHM";

        case WL_BUFFER_EGL:
        return "EGL";

        case WL_BUFFER_DMA_BUF:
        return "DMA-BUF";

        default:
        return "UNKNOWN";
    }
}

void surface_headless_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface || !buffer_resource) return;
    
    struct buffer *buffer = wl_resource_get_user_data(buffer_resource);
    SERVER_DEBUG("Called attach buffer with type: %s, size: %i or %iX%i\nBuffer data pointer: %s", buffer_type_to_string(buffer), buffer->size, buffer->width, buffer->height, buffer->shm.data);
}

static void compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct server *server = wl_resource_get_user_data(resource);
    
    struct wl_resource *surface_resource = wl_resource_create(
        client, &wl_surface_interface, wl_resource_get_version(resource), id);
    
    if (!surface_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    // Создаем структуру поверхности
    struct surface *surface = calloc(1, sizeof(struct surface));
    if (!surface) {
        wl_client_post_no_memory(client);
        wl_resource_destroy(surface_resource);
        return;
    }
    
    surface->resource = surface_resource;
    // surface->buffer = NULL;
    surface->server = server;
    surface->xdg_surface = NULL;
    surface->xdg_toplevel = NULL;
    wl_list_init(&surface->link);
    
    wl_resource_set_implementation(surface_resource, &surface_implementation, surface, NULL); // TODO: destructor
    
    wl_list_insert(&server->surfaces, &surface->link);
}

// REGIONS
static void compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct wl_resource *region_resource = wl_resource_create(
        client, &wl_region_interface, 1, id);
    
    if (!region_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    SERVER_DEBUG("Region created");
}

// COMPOSITOR
static const struct wl_compositor_interface compositor_implementation = {
    .create_surface = compositor_create_surface,
    .create_region = compositor_create_region,
};

void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id) {    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_compositor_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    wl_resource_set_implementation(resource, &compositor_implementation, data, NULL);
}