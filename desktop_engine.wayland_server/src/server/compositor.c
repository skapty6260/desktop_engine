#include "compositor.h"
#include "server.h"
#include "../logger/logger.h"
#include <stdlib.h>


/*  WL_SURFACE DESCRIPTION
    A surface is a rectangular area that may be displayed on zero or more outputs,
    and shown any number of times at the compositor's discretion. It can present
    wl_buffers, receive user input, and define a local coordinate system.

    Surface size and relative positions are described in surface-local coordinates,
    which may differ from buffer pixel coordinates when buffer_transform or
    buffer_scale is used.

    A surface without a role is useless - the compositor doesn't know where, when
    or how to present it. The role defines the surface's purpose. 
    Role Examples include:
    - a pointer cursor (set by wl_pointer.set_cursor),
    - a drag icon (wl_data_device.start_drag),
    - a sub-surface (wl_subcompositor.get_subsurface),
    - a window via shell protocols (XDG).

    Key rules:
    - Surface can have only one role at a time
    - Initially surface has no role
    - Once set, role is permanent for the surface's lifetime
    - Re-setting the same role is allowed unless explicitly forbidden
    - Role objects (like wl_subsurface) must be destroyed before the surface
    - Destroying role object stops the role execution but doesn't remove the role
    - Role switching between different types is prohibited
*/

/*  WL_SURFACE destroy

    DESCRIPTION
    Remove surface link from server->surfaces
    Cleanup buffers and regions
*/
static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    wl_list_remove(&surface->link);
    free(surface);

    wl_resource_destroy(resource);
    SERVER_DEBUG("COMPOSITOR_SURFACE surface destroyed: removed from server list, freed, destroyed surface resource");
}

/*  WL_SURFACE attach
Argument        Type           Description
        buffer	object<wl_buffer>	   buffer of surface contents
        x	    int	                   surface-local x coordinate
        y	    int	                   surface-local y coordinate
*/ 
static void surface_attach(struct wl_client *client, struct wl_resource *resource, struct wl_buffer *buffer, int x, int y) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE ATTACH CALLED");
}

/*  WL_SURFACE damage
*/ 
static void surface_damage(struct wl_client *client, struct wl_resource *resource, int x, int y, int width, int height) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE DAMAGE CALLED");
}

/*  WL_SURFACE frame
*/ 
static void surface_frame(struct wl_client *client, struct wl_resource *resource, struct wl_callback *callback) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE FRAME CALLED");
}

/*  WL_SURFACE set_opaque_region
*/ 
static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_region *region) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_OPAQUE_REGION CALLED");
}

/*  WL_SURFACE set_input_region 
*/ 
static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_region *region) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_INPUT_REGION CALLED");
}

/*  WL_SURFACE commit
*/ 
static void surface_commit(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE COMMIT CALLED");\
}

/*  WL_SURFACE set_buffer_transform
*/ 
static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_BUFFER_TRANSFORM CALLED");
}

/*  WL_SURFACE set_buffer_scale
*/ 
static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int scale) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE SET_BUFFER_SCALE CALLED");
}

/*  WL_SURFACE damage_buffer
*/ 
static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int x, int y, int width, int height) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE DAMAGE BUFFER CALLED");
}

/*  WL_SURFACE offset
*/ 
static void surface_offset(struct wl_client *client, struct wl_resource *resource, int x, int y) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE OFFSET CALLED");
}

/*  WL_SURFACE IMPLEMENTATION
    REQUEST destroy()
    REQUEST attach(buffer: object<wl_buffer>, x: int, y: int)
        Argument        Type           Description
        buffer	object<wl_buffer>	   buffer of surface contents
        x	    int	                   surface-local x coordinate
        y	    int	                   surface-local y coordinate
    REQUEST damage(x: int, y: int, width: int, height: int)
        Argument        Type        Description
        x	            int	        surface-local x coordinate
        y	            int	        surface-local y coordinate
        width	        int	        width of damage rectangle
        height	        int	        height of damage rectangle
    REQUEST frame(callback: new_id<wl_callback>)
        Argument            Type            Description
        callback	new_id<wl_callback>	    callback object for the frame request
    REQUEST set_opaque_region(region: object<wl_region>)
        Argument            Type           Description
        region	    object<wl_region>	   opaque region of the surface
    REQUEST set_input_region(region: object<wl_region>)
        Argument            Type           Description
        region	    object<wl_region>	   input region of the surface
    REQUEST commit()
    REQUEST set_buffer_transform(transform: int<wl_output.transform>)
        Argument            Type                Description
        transform	int<wl_output.transform>	transform for interpreting buffer contents
    REQUEST set_buffer_scale(scale: int)
        Argument        Type        Description
        scale	        int	        scale for interpreting buffer contents
    REQUEST damage_buffer(x: int, y: int, width: int, height: int)
        Argument        Type        Description
        x	            int	        bufer-local x coordinate
        y	            int	        bufer-local y coordinate
        width	        int	        width of damage rectangle
        height	        int	        height of damage rectangle
    REQUEST offset(x: int, y: int)
        Argument        Type        Description
        x	            int	        surface-local x coordinate
        y	            int	        surface-local y coordinate
*/
static const struct wl_surface_interface surface_implementation = {
    .destroy = surface_destroy,
    .attach = surface_attach,
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


/*  WL_COMPOSITOR DESCRIPTION
    A compositor. This object is a singleton global.
    The compositor is in charge of combining the contents of multiple surfaces
    into one displayable output.
*/


//  WL_COMPOSITOR create_surface
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
    wl_list_init(&surface->link);
    
    // Добавляем поверхность в список сервера
    wl_list_insert(&server->surfaces, &surface->link);
    
    wl_resource_set_implementation(surface_resource, &surface_implementation, surface, NULL);
    
    SERVER_DEBUG("Surface created");
}

//  WL_COMPOSITOR create_region
static void compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct wl_resource *region_resource = wl_resource_create(
        client, &wl_region_interface, 1, id);
    
    if (!region_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    SERVER_DEBUG("Region created");
}

/*  WL_COMPOSITOR IMPLEMENTATION
    REQUEST create_surface(id: new_id<wl_surface>)
        Argument            Type            Description
        id	        new_id<wl_surface>	    the new surface  
    REQUEST create_region(id: new_id<wl_region>)
        Argument        Type                Description
        id	        new_id<wl_region>	    the new region
*/
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
    
    SERVER_DEBUG("Compositor bound to client");
}