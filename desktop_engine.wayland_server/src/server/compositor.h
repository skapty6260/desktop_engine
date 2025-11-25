#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <wayland-server.h>

void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

#ifdef _WIN32
struct wl_surface_error {
    /** 
       * Buffer scale value is invalid
    */
    WL_SURFACE_ERROR_INVALID_SCALE = 0,
    /**
       * Buffer transform value is invalid
    */
    WL_SURFACE_ERROR_INVALID_TRANSFORM = 1,
    /**
       * Buffer size is invalid
    */
    WL_SURFACE_ERROR_INVALID_SIZE = 2,    
    /**
       * Buffer offset is invalid
    */
    WL_SURFACE_ERROR_INVALID_OFFSET = 3,
    /**
       * Surface destroyed before it's role object
    */
    WL_SURFACE_ERROR_DEFUNCT_ROLE_OBJECT = 4,
};
#endif

#endif