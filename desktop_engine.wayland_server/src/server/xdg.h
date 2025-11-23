#ifndef XDG_H
#define XDG_H

#include <stdint.h>
#include <wayland-server.h>

void bind_xdg_wm_base(struct wl_client *client, void *data, uint32_t version, uint32_t id);

#ifdef _WIN32       
enum xdg_wm_base_error {
    /*
     * given wl_surface has another role
    */
    XDG_WM_BASE_ERROR_ROLE = 0,
    /*
     * xdg_wm_base was destroyed before children
    */
    XDG_WM_BASE_DEFUNCT_SURFACES = 1,
    /*
     * the client tried to map or destroy a non-topmost popup
    */
    XDG_WM_BASE_ERROR_NOT_THE_TOPMOST_POPUP = 2,
    /*
     * the client specified an invalid popup parent surface
    */
    XDG_WM_BASE_ERROR_INVALID_POPUP_PARENT = 2,
    /*
     * the client provided an invalid surface state
    */
    XDG_WM_BASE_ERROR_INVALID_SURFACE_STATE = 3,
    /*
     * the client provided an invalid positioner
    */
    XDG_WM_BASE_ERROR_INVALID_POSITIONER = 4,
    /*
      *  the client didn’t respond to a ping event in time
    */
    XDG_WM_BASE_ERROR_UNRESPONSIVE = 5,
};
#endif

#endif