#include "server.h"
#include "../logger/logger.h"
#include "shm.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

static void shm_buffer_destroy(struct wl_resource *resource) {
    struct shm_buffer *buffer = wl_resource_get_user_data(resource);
    
    if (buffer) {
        if (buffer->pool) {
            wl_list_remove(&buffer->link);
        }
        SERVER_DEBUG("SHM buffer destroyed: %dx%d", buffer->width, buffer->height);
        free(buffer);
    }
}

static void buffer_handle_destroy(struct wl_client *client, struct wl_resource *resource) {
    SERVER_DEBUG("Buffer destroy requested by client");
    // Ресурс будет уничтожен автоматически через destructor
}

static const struct wl_buffer_interface buffer_implementation = {
    .destroy = buffer_handle_destroy,
};

static void shm_pool_destroy(struct wl_client *client, struct wl_resource *pool_resource) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);

    if (pool) {
        SERVER_DEBUG("Destroying SHM pool: fd=%d, size=%zu", pool->fd, pool->size);
        
        // Уничтожаем все буферы в этом пуле
        struct shm_buffer *buffer, *tmp;
        wl_list_for_each_safe(buffer, tmp, &pool->buffers, link) {
            if (buffer->resource) {
                wl_resource_destroy(buffer->resource);
            }
        }
        
        // Освобождаем mmap'ed память
        if (pool->data && pool->data != MAP_FAILED) {
            munmap(pool->data, pool->size);
        }
        
        // Закрываем файловый дескриптор
        if (pool->fd >= 0) {
            close(pool->fd);
        }
        
        // Удаляем из глобального списка сервера
        wl_list_remove(&pool->link);
        free(pool);
    }
    
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
            return true;
        default:
            return false;
    }
}

static void shm_pool_create_buffer(struct wl_client *client, struct wl_resource *pool_resource, 
                                  uint32_t id, int32_t offset, int32_t width, int32_t height, 
                                  int32_t stride, uint32_t format) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);
    SERVER_DEBUG("CALLED shm_pool_create_buffer");

    if (!pool) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_FD, "invalid pool");
        return;
    }

    // Валидация параметров
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
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, 
                              "buffer exceeds pool size");
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

    // Создаем нашу структуру для отслеживания
    struct shm_buffer *buffer = calloc(1, sizeof(struct shm_buffer));
    if (!buffer) {
        wl_resource_destroy(buffer_resource);
        wl_client_post_no_memory(client);
        return;
    }

    buffer->resource = buffer_resource;
    buffer->pool = pool;
    buffer->offset = offset;
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride;
    buffer->format = format;

    // ВАЖНО: Регистрируем буфер как SHM буфер в Wayland
    // Это позволяет wl_shm_buffer_get() находить его
    wl_shm_buffer_set_user_data(buffer_resource, buffer);

    wl_list_init(&buffer->link);
    wl_list_insert(&pool->buffers, &buffer->link);

    // Устанавливаем реализацию
    wl_resource_set_implementation(buffer_resource, &buffer_implementation, buffer, shm_buffer_destroy);

    SERVER_DEBUG("SHM buffer created: %dx%d, stride=%d, format=0x%x, offset=%d",
                width, height, stride, format, offset);
}

static void shm_pool_resize(struct wl_client *client, struct wl_resource *pool_resource, int32_t size) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);
    
    if (!pool) {
        return;
    }

    if (size <= pool->size) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE,
                              "can only resize to larger size");
        return;
    }

    // Обновляем mmap если нужно
    if (pool->data && pool->data != MAP_FAILED) {
        munmap(pool->data, pool->size);
        pool->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, pool->fd, 0);
        if (pool->data == MAP_FAILED) {
            wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_FD,
                                  "failed to remap shared memory");
            return;
        }
    }
    
    pool->size = size;
    SERVER_DEBUG("SHM pool resized to %d bytes", size);
}

static const struct wl_shm_pool_interface shm_pool_implementation = {
    .create_buffer = shm_pool_create_buffer,
    .destroy = shm_pool_destroy,
    .resize = shm_pool_resize,
};

static void shm_create_pool(struct wl_client *client, struct wl_resource *shm_resource, uint32_t id, int fd, int32_t size) {
    struct server *server = wl_resource_get_user_data(shm_resource);

    SERVER_DEBUG("CALLED shm_create_pool");

    if (size <= 0) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_STRIDE, "invalid size");
        close(fd);
        return;
    }
    
    if (fd < 0) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD, "invalid file descriptor");
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

    // Mmap shared memory для доступа к данным
    pool->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool->data == MAP_FAILED) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD,
                              "failed to mmap shared memory");
        close(fd);
        free(pool);
        return;
    }

    // Создаем ресурс пула
    pool->resource = wl_resource_create(client, &wl_shm_pool_interface, 1, id);
    if (!pool->resource) {
        munmap(pool->data, size);
        close(fd);
        free(pool);
        return;
    }

    wl_resource_set_implementation(pool->resource, &shm_pool_implementation, pool, NULL);
    wl_list_insert(&server->shm_pools, &pool->link);
    
    SERVER_DEBUG("SHM pool created successfully: fd=%d, size=%d", fd, size);
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
    
    SERVER_DEBUG("SHM bound to client");

    // Отправляем поддерживаемые форматы клиенту
    wl_shm_send_format(resource, WL_SHM_FORMAT_ARGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_XRGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_RGBA8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_BGRA8888);
}