#include <xdg-shell/surface.h>
#include <xdg-shell/toplevel.h>
#include <logger.h>

#include "xdg-shell-protocol.h"

void xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    wl_resource_destroy(resource);
}

void xdg_surface_get_toplevel(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct surface *surface = wl_resource_get_user_data(resource);
    
    struct wl_resource *toplevel = wl_resource_create(client, &xdg_toplevel_interface, 1, id);
    if (!toplevel) {
        wl_client_post_no_memory(client);
        return;
    }
    
    surface->xdg_toplevel = toplevel;
    wl_resource_set_implementation(toplevel, &xdg_toplevel_implementation, surface, NULL);
    
    // Отправляем configure для toplevel с правильными параметрами
    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(toplevel, 400, 300, &states);
    wl_array_release(&states);
    
    // Отправляем configure для surface
    xdg_surface_send_configure(surface->xdg_surface, 1); 

    SERVER_DEBUG("XDG toplevel created");
}

void xdg_surface_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    SERVER_DEBUG("XDG_SURFACE: set window geometry requested");
}

void xdg_surface_get_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *parent, struct wl_resource *positioner) {
    SERVER_DEBUG("XDG_SURFACE: popup requested");
}

void xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    SERVER_DEBUG("XDG_SURFACE ack configure: %u", serial);
}

const struct xdg_surface_interface xdg_surface_implementation = {
    .destroy = xdg_surface_destroy,
    .get_toplevel = xdg_surface_get_toplevel,
    .get_popup = xdg_surface_get_popup,
    .set_window_geometry = xdg_surface_set_window_geometry,
    .ack_configure = xdg_surface_ack_configure,
};