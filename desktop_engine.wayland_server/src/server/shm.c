#include "server.h"
#include "../logger/logger.h"
#include "shm.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

/*
    WL_SHM_POOL
    REQUEST create_buffer(id: new_id<wl_buffer>, offset: int, width: int, height: int, stride: int, format: uint<wl_shm.format>)
        Argument        Type         Description
        id	     new_id<wl_buffer>   buffer to create
        offset	 int	             buffer byte offset within the pool
        width	 int	             buffer width, in pixels
        height	 int	             buffer height, in pixels
        stride	 int	             number of bytes from the beginning of one row to the beginning
                                     of the next row
        format	 uint<wl_shm.format> buffer pixel format
    create a buffer from the pool
    Create a wl_buffer object from the pool.
    The buffer is created offset bytes into the pool and has width and height as specified.
    The stride argument specifies the number of bytes from the beginning of one row to the 
    beginning of the next. The format is the pixel format of the buffer and must be one of those
    advertised through the wl_shm.format event.
    A buffer will keep a reference to the pool it was created from so it is valid to destroy the
    pool immediately after creating a buffer from it.

    REQUEST destroy()
    destroy the pool
    Destroy the shared memory pool.
    The mmapped memory will be released when all buffers that have been created from this pool
    are gone.

    REQUEST resize(size: int)
        Argument        Type        Description
        size	        int	        new size of the pool, in bytes
    change the size of the pool mapping
    This request will cause the server to remap the backing memory for the pool from the file 
    descriptor  passed when the pool was created, but using the new size. This request can only be
    used to make the pool bigger.
    This request only changes the amount of bytes that are mmapped by the server and does not
    touch the file corresponding to the file descriptor passed at creation time. It is the client's
    responsibility to ensure that the file is at least as big as the new pool size.
*/

static void shm_buffer_destroy(struct wl_resource *resource) {
    struct shm_buffer *buffer = wl_resource_get_user_data(resource);
    
    if (buffer && buffer->pool) {
        // Remove buffer from pool's list
        wl_list_remove(&buffer->link);
        SERVER_DEBUG("SHM buffer destroyed: %dx%d", buffer->width, buffer->height);
    }
    free(buffer);
}

static void shm_pool_destroy(struct wl_client *client, struct wl_resource *pool_resource) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);

    if (pool) {
        SERVER_DEBUG("Destroying SHM pool: fd=%d, size=%zu", pool->fd, pool->size);
        
        // Destroy all buffers in this pool
        struct shm_buffer *buffer, *tmp;
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
    struct shm_buffer *buffer = wl_resource_get_user_data(resource);
    SERVER_DEBUG("Buffer destroy requested by client");
    
    if (buffer) {
        // Освобождаем ресурсы буфера
        free(buffer);
    }
}

static const struct wl_buffer_interface buffer_implementation = {
    .destroy = buffer_handle_destroy,
};

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
    
    // Более гибкая проверка stride
    int min_stride = width * 4; // минимальный stride для 32bpp
    if (stride < min_stride) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, 
                              "stride too small (min: %d, got: %d)", min_stride, stride);
        return;
    }

    // Проверка что буфер помещается в pool
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

    // Создаем структуру буфера
    struct shm_buffer *buffer = calloc(1, sizeof(struct shm_buffer));
    if (!buffer) {
        wl_client_post_no_memory(client);
        return;
    }

    // Создаем ресурс буфера
    buffer->resource = wl_resource_create(client, &wl_buffer_interface, 1, id);
    if (!buffer->resource) {
        wl_client_post_no_memory(client);
        free(buffer);
        return;
    }

    buffer->pool = pool;
    buffer->offset = offset;
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride;
    buffer->format = format;
    
    // Вычисляем указатель на данные
    buffer->data = (unsigned char*)pool->data + offset;

    wl_list_init(&buffer->link);
    wl_list_insert(&pool->buffers, &buffer->link);

    static const struct wl_buffer_interface buffer_impl = {
        .destroy = buffer_handle_destroy,
    };
    wl_resource_set_implementation(buffer->resource, &buffer_impl, buffer, shm_buffer_destroy);

    SERVER_DEBUG("SHM buffer created: %dx%d, stride=%d, format=0x%x, offset=%d, data=%p",
                width, height, stride, format, offset, buffer->data);

    // НЕ отправляем release сразу - это должен делать композитор после рендеринга
    // wl_buffer_send_release(buffer->resource); // ⛔️ УБЕРИТЕ ЭТУ СТРОКУ
}

// TODO
static void shm_pool_resize(struct wl_client *client, struct wl_resource *pool_resource, int32_t size) {
    SERVER_DEBUG("SHM_POOL RESIZED");
}

static const struct wl_shm_pool_interface shm_pool_implementation = {
    .create_buffer = shm_pool_create_buffer,
    .destroy = shm_pool_destroy,
    .resize = shm_pool_resize,
};

/*
    WL_SHM
    REQUEST create_pool(id: new_id<wl_shm_pool>, fd: fd, size: int)
        Argument            Type            Description
        id	           new_id<wl_shm_pool>  pool to create
        fd	           fd	                file descriptor for the pool
        size	       int	                pool size, in bytes
    Create a new wl_shm_pool object.
    The pool can be used to create shared memory based buffer objects. The server will mmap size 
    bytes of the passed file descriptor, to use as backing memory for the pool.

    REQUEST release()
    release the shm object
    Using this request a client can tell the server that it is not going to use the shm object
    anymore.
    Objects created via this interface remain unaffected.

    EVENT format(format: uint<wl_shm.format>)
        Argument            Type            Description
        format	       uint<wl_shm.format>	buffer pixel format
    pixel format description
    Informs the client about a valid pixel format that can be used for buffers.
    Known formats include argb8888 and xrgb8888.

    ENUM error { invalid_format, invalid_stride, invalid_fd } 
        Argument       Value        Description
        invalid_format	0	        buffer format is not known
        invalid_stride	1	        invalid size or stride during pool or buffer creation
        invalid_fd	    2	        mmapping the file descriptor failed
    wl_shm error values
    These errors can be emitted in response to wl_shm requests.

    ENUM format { argb8888, xrgb8888, c8, rgb332, bgr233, xrgb4444, ... }
    pixel formats
    This describes the memory layout of an individual pixel.
    All renderers should support argb8888 and xrgb8888 but any other formats are optional 
    and may not be supported by the particular renderer in use.
    The drm format codes match the macros defined in drm_fourcc.h, except argb8888 and xrgb8888.
    The formats actually supported by the compositor will be reported by the format event.
    For all wl_shm formats and unless specified in another protocol extension, pre-multiplied
    alpha is used for pixel values.
*/

static void shm_create_pool(struct wl_client *client, struct wl_resource *shm_resource, uint32_t id, int fd, int32_t size) {
    struct server *server = wl_resource_get_user_data(shm_resource);

    SERVER_DEBUG("CALLED shm_create_pool");

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

    // Mmap shared memory for data access
    SERVER_DEBUG("shm_create_pool: mmap data");
    pool->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool->data == MAP_FAILED) {
        wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD,
                              "failed to mmap shared memory");
        close(fd);
        free(pool);
        return;
    }

    // Create pool resource
    SERVER_DEBUG("shm_create_pool: create_resource");
    pool->resource = wl_resource_create(client, &wl_shm_pool_interface, 1, id);
    if (!pool->resource) {
        wl_client_post_no_memory(client);
        munmap(pool->data, size);
        close(fd);
        free(pool);
        return;
    }
    
    SERVER_DEBUG("shm_create_pool: set implementation");
    wl_resource_set_implementation(pool->resource, &shm_pool_implementation, pool, NULL);

    SERVER_DEBUG("shm_create_pool: insert pool to server");
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

    // Send supported formats to client
    wl_shm_send_format(resource, WL_SHM_FORMAT_ARGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_XRGB8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_RGBA8888);
    wl_shm_send_format(resource, WL_SHM_FORMAT_BGRA8888);
}