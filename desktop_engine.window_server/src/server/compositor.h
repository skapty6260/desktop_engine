#include <stdint.h>
#include <wayland-server.h>
void bind_compositor(struct wl_client *client, void *data,
                           uint32_t version, uint32_t id);