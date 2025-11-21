#include "server.h"
#include "logger/logger.h"
#include "shm.h"

static void shm_create_pool(struct wl_client *client,
                           struct wl_resource *shm_resource,
                           uint32_t id, int fd, int32_t size) {
    SERVER_DEBUG("Creating SHM pool: fd=%d, size=%d", fd, size);
}

static const struct wl_shm_interface shm_implementation = {
    .create_pool = shm_create_pool,
};

void bind_shm(struct wl_client *client, void *data,
                            uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_shm_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &shm_implementation, data, NULL);
    
    // shm_send_formats(client, resource);
    
    SERVER_DEBUG("SHM bound to client");
}