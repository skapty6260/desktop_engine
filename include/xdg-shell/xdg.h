#ifndef XDG_H
#define XDG_H

#include <stdint.h>
#include <wayland-server.h>

#include "xdg-shell-protocol.h"

void bind_xdg_wm_base(struct wl_client *client, void *data, uint32_t version, uint32_t id);

#endif