#include <xdg-shell/wm_base.h>
#include <xdg-shell/surface.h>
#include <logger.h>

#include "xdg-shell-protocol.h"

static void xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource) {
    wl_resource_destroy(resource);
}

static void xdg_wm_base_create_positioner(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    // Basic implementation - create a positioner resource
    struct wl_resource *positioner = wl_resource_create(client, &xdg_positioner_interface, 1, id);
    if (!positioner) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(positioner, NULL, NULL, NULL);
    SERVER_DEBUG("XDG positioner created");
}

static void xdg_wm_base_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface) {
    struct server *server = wl_resource_get_user_data(resource);
    
    // Create xdg_surface resource
    struct wl_resource *xdg_surface = wl_resource_create(client, &xdg_surface_interface, 1, id);
    if (!xdg_surface) {
        wl_client_post_no_memory(client);
        return;
    }
    
    // Find the corresponding surface
    struct surface *surf = NULL;
    wl_list_for_each(surf, &server->surfaces, link) {
        if (surf->resource == surface) {
            break;
        }
    }
    
    if (!surf) {
        wl_resource_post_error(resource, XDG_WM_BASE_ERROR_INVALID_SURFACE_STATE, "invalid surface");
        wl_resource_destroy(xdg_surface);
        return;
    }
    
    // Store xdg_surface in surface structure
    surf->xdg_surface = xdg_surface;
    
    // Create implementation for xdg_surface
    wl_resource_set_implementation(xdg_surface, &xdg_surface_implementation, surf, NULL);
    
    SERVER_DEBUG("XDG surface created and configured");
}

static void xdg_wm_base_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    SERVER_DEBUG("XDG SHELL: pong received: %u", serial);
}

static const struct xdg_wm_base_interface xdg_wm_base_implementation = {
    .destroy = xdg_wm_base_destroy,
    .create_positioner = xdg_wm_base_create_positioner,
    .get_xdg_surface = xdg_wm_base_get_xdg_surface,
    .pong = xdg_wm_base_pong,
};

void bind_xdg_wm_base(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &xdg_wm_base_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &xdg_wm_base_implementation, server, NULL);
    
    xdg_wm_base_send_ping(resource, 1234);
    SERVER_DEBUG("XDG shell bound to client");
}