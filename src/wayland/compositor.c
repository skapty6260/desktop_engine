#include <wayland/compositor.h>
#include <logger.h>
#include <wayland/server.h>
#include <stdlib.h>

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

    SERVER_DEBUG("COMPOSITOR: Creating wl_surface id=%u, surface_implementation.attach=%p", 
                id, surface_implementation.attach);
    
    wl_list_insert(&server->surfaces, &surface->link);

    SERVER_DEBUG("COMPOSITOR: Surface created: resource=%p, added to server list", surface_resource);
}

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