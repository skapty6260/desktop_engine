#include <server/server.h>
#include <logger/logger.h>
#include <server/wayland/shm.h>
#include <server/wayland/buffer.h>

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

static void shm_buffer_destroy(struct wl_resource *resource) {
    struct buffer *buffer = wl_resource_get_user_data(resource);
    
    if (buffer) {
        // Remove buffer from pool's list
        wl_list_remove(&buffer->link);
        SERVER_DEBUG("SHM buffer destroyed: %dx%d", buffer->width, buffer->height);
        
        // Free the buffer structure
        free(buffer);
    }
}

void destroy_shm_pool(void *data) {
    struct shm_pool *pool = data;

    if (pool) {
        SERVER_DEBUG("Destroying SHM pool: fd=%d, size=%zu", pool->fd, pool->size);
        
        // Destroy all buffers in this pool
        struct buffer *buffer, *tmp;
        wl_list_for_each_safe(buffer, tmp, &pool->buffers, link) {
            if (buffer->resource) {
                wl_resource_destroy(buffer->resource);
            }
        }
        
        // Free mmap'ed memory
        if (pool->data && pool->data != MAP_FAILED) {
            munmap(pool->data, pool->size);
        }
        
        // Close file descriptor
        if (pool->fd >= 0) {
            close(pool->fd);
        }
        
        // Remove from server's global list
        wl_list_remove(&pool->link);
        free(pool);
    }
}

static void shm_pool_destroy(struct wl_client *client, struct wl_resource *pool_resource) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);
    destroy_shm_pool(pool);
    wl_resource_destroy(pool_resource);
}

static bool shm_pool_check_format(uint32_t format) {
    switch (format) {
        case WL_SHM_FORMAT_ARGB8888:
        case WL_SHM_FORMAT_XRGB8888:
        case WL_SHM_FORMAT_RGBA8888:
        case WL_SHM_FORMAT_BGRA8888:
        case WL_SHM_FORMAT_ABGR8888:
        case WL_SHM_FORMAT_XBGR8888:
            // Поддерживаемые форматы
            return true;
        default:
            return false;
    }
}

static void buffer_handle_destroy(struct wl_client *client, struct wl_resource *resource) {
    struct buffer *buffer = wl_resource_get_user_data(resource);
    SERVER_DEBUG("Buffer destroy requested by client");
    
    if (buffer) {
        // Note: Actual destruction happens in shm_buffer_destroy destructor
        // We just destroy the Wayland resource here, which will trigger shm_buffer_destroy
        wl_resource_destroy(resource);
    }
}

static const struct wl_buffer_interface buffer_implementation = {
    .destroy = buffer_handle_destroy,
};

static void shm_pool_create_buffer(struct wl_client *client, struct wl_resource *pool_resource, 
                                  uint32_t id, int32_t offset, int32_t width, int32_t height, 
                                  int32_t stride, uint32_t format) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);

    if (!pool) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_FD, "invalid pool");
        return;
    }

    if (offset < 0 || (size_t)offset >= pool->size) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, "invalid offset");
        return;
    }

    if (width <= 0 || height <= 0) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, "invalid width or height");
        return;
    }
    
    int min_stride = width * 4;
    if (stride < min_stride) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, 
                              "stride too small (min: %d, got: %d)", min_stride, stride);
        return;
    }

    size_t required_size = (size_t)offset + (size_t)height * (size_t)stride;
    if (required_size > pool->size) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, "buffer exceeds pool size");
        return;
    }

    if (!shm_pool_check_format(format)) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_FORMAT, "unsupported format");
        return;
    }

    // Создаем ресурс буфера
    struct wl_resource *buffer_resource = wl_resource_create(client, &wl_buffer_interface, 1, id);
    if (!buffer_resource) {
        wl_client_post_no_memory(client);
        return;
    }

    struct buffer *buffer = buffer_create_shm(buffer_resource, pool->data, id, offset, width, height, stride, format);
    if (!buffer) {
        wl_client_post_no_memory(client);
        wl_resource_destroy(buffer_resource);
        return;
    }
    
    // Вычисляем указатель на данные
    // buffer->data = (unsigned char*)pool->data + offset;

    wl_list_init(&buffer->link);
    wl_list_insert(&pool->buffers, &buffer->link);

    static const struct wl_buffer_interface buffer_impl = {
        .destroy = buffer_handle_destroy,
    };
    wl_resource_set_implementation(buffer->resource, &buffer_impl, buffer, shm_buffer_destroy);

    SERVER_DEBUG("SHM buffer created: %dx%d, stride=%d, format=0x%x, offset=%d=",
                width, height, stride, format, offset);

    // НЕ отправляем release сразу - это должен делать композитор после рендеринга
    // wl_buffer_send_release(buffer->resource); // ⛔️ УБЕРИТЕ ЭТУ СТРОКУ
}

static void shm_pool_resize(struct wl_client *client, struct wl_resource *pool_resource, int32_t size) {
    SERVER_DEBUG("SHM_POOL RESIZED");
}

static const struct wl_shm_pool_interface shm_pool_implementation = {
    .create_buffer = shm_pool_create_buffer,
    .destroy = shm_pool_destroy,
    .resize = shm_pool_resize,
};

static void shm_create_pool(struct wl_client *client, struct wl_resource *shm_resource, uint32_t id, int fd, int32_t size) {
    struct server *server = wl_resource_get_user_data(shm_resource);

    if (size <= 0) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_STRIDE,
                              "invalid size");
        close(fd);
        return;
    }
    
    if (fd < 0) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD,
                              "invalid file descriptor");
        return;
    }

    struct shm_pool *pool = calloc(1, sizeof(struct shm_pool));
    if (!pool) {
        wl_client_post_no_memory(client);
        close(fd);
        return;
    }

    pool->fd = fd;
    pool->size = size;
    wl_list_init(&pool->buffers);

    pool->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool->data == MAP_FAILED) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD,
                              "failed to mmap shared memory");
        close(fd);
        free(pool);
        return;
    }

    pool->resource = wl_resource_create(client, &wl_shm_pool_interface, 1, id);
    if (!pool->resource) {
        wl_client_post_no_memory(client);
        munmap(pool->data, size);
        close(fd);
        free(pool);
        return;
    }
    
    wl_resource_set_implementation(pool->resource, &shm_pool_implementation, pool, NULL);
    wl_list_insert(&server->shm_pools, &pool->link);
}

static const struct wl_shm_interface shm_implementation = {
    .create_pool = shm_create_pool,
};

void bind_shm(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_shm_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &shm_implementation, data, NULL);

    // Send supported formats to client
    wl_shm_send_format(resource, WL_SHM_FORMAT_ARGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_XRGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_RGBA8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_BGRA8888);
}