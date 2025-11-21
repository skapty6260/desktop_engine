#include "server.h"
#include "logger/logger.h"
#include "xdg.h"
#include "xdg-shell-protocol.h"

static void xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    wl_resource_destroy(resource);
}

static void xdg_surface_get_toplevel(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct surface *surface = wl_resource_get_user_data(resource);
    
    struct wl_resource *toplevel = wl_resource_create(client, &xdg_toplevel_interface, 1, id);
    if (!toplevel) {
        wl_client_post_no_memory(client);
        return;
    }
    
    surface->xdg_toplevel = toplevel;
    wl_resource_set_implementation(toplevel, NULL, surface, NULL);
    
    xdg_toplevel_set_title(toplevel, "Wayland Client");
    SERVER_DEBUG("XDG toplevel created");
}

static void xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    SERVER_DEBUG("XDG surface ack configure: %u", serial);
}

static const struct xdg_surface_interface xdg_surface_implementation = {
    .destroy = xdg_surface_destroy,
    .get_toplevel = xdg_surface_get_toplevel,
    .ack_configure = xdg_surface_ack_configure,
};

/*
    XDG_WM_BASE
    REQUEST destroy()
    destroy xdg_wm_base
    Destroy this xdg_wm_base object.
    Destroying a bound xdg_wm_base object while there are surfaces still alive created by this
    xdg_wm_base object instance is illegal and will result in a defunct_surfaces error.

    REQUEST create_positioner(id: new_id<xdg_positioner>)
        Argument        Type                Description
        id	        new_id<xdg_surface>	
        surface	    object<wl_surface>	
    create a positioner object
    Create a positioner object. A positioner object is used to position surfaces relative to some
    parent surface.
    See the interface description and xdg_surface.get_popup for details.

    REQUEST get_xdg_surface(id: new_id<xdg_surface>, surface: object<wl_surface>)
        Argument        Type                Description
        id	        new_id<xdg_surface>	
        surface	    object<wl_surface>
    create a shell surface from a surface
    This creates an xdg_surface for the given surface. While xdg_surface itself is not a role,
    the corresponding surface may only be assigned a role extending xdg_surface, such as
    xdg_toplevel or xdg_popup. It is illegal to create an xdg_surface for a wl_surface which
    already has an assigned role and this will result in a role error.
    This creates an xdg_surface for the given surface. An xdg_surface is used as basis to define
    a role to a given surface, such as xdg_toplevel or xdg_popup. It also manages functionality
    shared between xdg_surface based surface roles.

    REQUEST pong(serial: uint)
        Argument        Type        Description
        serial	        uint        serial of the ping event
    respond to a ping event
    A client must respond to a ping event with a pong request or the client may be deemed
    unresponsive. See xdg_wm_base.ping and xdg_wm_base.error.unresponsive.

    ENUM error { role, defunct_surfaces, not_the_topmost_popup, invalid_popup_parent, ... }
        Argument                Value        Description
        role	                0	         given wl_surface has another role
        defunct_surfaces	    1	         xdg_wm_base was destroyed before children
        not_the_topmost_popup	2	         the client tried to map or destroy a non-topmost popup
        invalid_popup_parent	3	         the client specified an invalid popup parent surface
        invalid_surface_state	4	         the client provided an invalid surface state
        invalid_positioner	    5  	         the client provided an invalid positioner
        unresponsive	        6	         the client didn’t respond to a ping event in time
*/

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
    
    // Send configure event
    xdg_surface_send_configure(xdg_surface, 1); // serial
    
    SERVER_DEBUG("XDG surface created and configured");
}

static const struct xdg_wm_base_interface xdg_wm_base_implementation = {
    .destroy = xdg_wm_base_destroy,
    .create_positioner = xdg_wm_base_create_positioner,
    .get_xdg_surface = xdg_wm_base_get_xdg_surface,
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