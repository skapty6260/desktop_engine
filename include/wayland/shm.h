#ifndef SHM_H
#define SHM_H

#include <stdint.h>
#include <wayland-server.h>

struct shm_pool {
    struct wl_resource *resource;
    struct wl_list link;
    int fd;                 
    size_t size;            
    void *data;             
    struct wl_list buffers;
};

void bind_shm(struct wl_client *client, void *data, uint32_t version, uint32_t id);

#endif