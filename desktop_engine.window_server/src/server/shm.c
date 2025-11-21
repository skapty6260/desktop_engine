#include "server.h"
#include "logger/logger.h"
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

static void shm_pool_destroy(struct wl_client *client, struct wl_resource *pool_resource) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);

    if (pool) {
        SERVER_DEBUG("Destroying SHM pool: fd=%d, size=%zu", pool->fd, pool->size);
        
        // Уничтожаем все буферы этого пула
        struct shm_buffer *buffer, *tmp;
        wl_list_for_each_safe(buffer, tmp, &pool->buffers, link) {
            wl_list_remove(&buffer->link);
            if (buffer->resource) {
                wl_resource_destroy(buffer->resource);
            }
            free(buffer);
        }
        
        // Освобождаем mmap'нутую память
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
            // Поддерживаемые форматы
            return true;
        default:
            return false;
    }
}

static void shm_pool_create_buffer(struct wl_client *client, struct wl_resource *pool_resource, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) {
    struct shm_pool *pool = wl_resource_get_user_data(pool_resource);
    SERVER_DEBUG("CALLED shm_pool_create_buffer");

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
    
    if (stride < width * 4) { // minimal stride for 32bpp
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, "invalid stride");
        return;
    }

    // Check if buffer fits in pool
    size_t required_size = offset + (size_t)height * (size_t)stride;
    if (required_size > pool->size) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_STRIDE, "buffer exceeds pool size");
        return;
    }

    SERVER_DEBUG("shm_pool_create_buffer: check format");
    if (shm_pool_check_format(format) == false) {
        wl_resource_post_error(pool_resource, WL_SHM_ERROR_INVALID_FORMAT, "unsupported format");
        return;
    }

    // Create buffer structure
    struct shm_buffer *buffer = calloc(1, sizeof(struct shm_buffer));
    if (!buffer) {
        wl_client_post_no_memory(client);
        return;
    }
    

    SERVER_DEBUG("shm_pool_create_buffer: create resource");
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

    // Add buffer to pool list
    SERVER_DEBUG("shm_pool_create_buffer: list insert");
    wl_list_insert(&pool->buffers, &buffer->link);

    // Setup buffer implementation
    SERVER_DEBUG("shm_pool_create_buffer: setup null implementation");
    wl_resource_set_implementation(buffer->resource, NULL, buffer, NULL);

    // Setup buffer destructor
    SERVER_DEBUG("shm_pool_create_buffer: set null destructor");
    wl_resource_set_destructor(buffer->resource, NULL);

    SERVER_DEBUG("SHM buffer created: %dx%d, stride=%d, format=0x%x, offset=%d",
                width, height, stride, format, offset);

    // TODO
    // В реальном композиторе wl_buffer_send_release() вызывается после рендеринга
    // Для демонстрации отправляем сразу
    wl_buffer_send_release(buffer->resource);
}

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

    ENUM format { argb8888, xrgb8888, c8, rgb332, bgr233, xrgb4444, xbgr4444, rgbx4444, bgrx4444, argb4444, abgr4444, rgba4444, bgra4444, xrgb1555, xbgr1555, rgbx5551, bgrx5551, argb1555, abgr1555, rgba5551, bgra5551, rgb565, bgr565, rgb888, bgr888, xbgr8888, rgbx8888, bgrx8888, abgr8888, rgba8888, bgra8888, xrgb2101010, xbgr2101010, rgbx1010102, bgrx1010102, argb2101010, abgr2101010, rgba1010102, bgra1010102, yuyv, yvyu, uyvy, vyuy, ayuv, nv12, nv21, nv16, nv61, yuv410, yvu410, yuv411, yvu411, yuv420, yvu420, yuv422, yvu422, yuv444, yvu444, r8, r16, rg88, gr88, rg1616, gr1616, xrgb16161616f, xbgr16161616f, argb16161616f, abgr16161616f, xyuv8888, vuy888, vuy101010, y210, y212, y216, y410, y412, y416, xvyu2101010, xvyu12_16161616, xvyu16161616, y0l0, x0l0, y0l2, x0l2, yuv420_8bit, yuv420_10bit, xrgb8888_a8, xbgr8888_a8, rgbx8888_a8, bgrx8888_a8, rgb888_a8, bgr888_a8, rgb565_a8, bgr565_a8, nv24, nv42, p210, p010, p012, p016, axbxgxrx106106106106, nv15, q410, q401, xrgb16161616, xbgr16161616, argb16161616, abgr16161616, c1, c2, c4, d1, d2, d4, d8, r1, r2, r4, r10, r12, avuy8888, xvuy8888, p030 }
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
    
    wl_resource_set_implementation(pool->resource, &shm_pool_implementation, pool, NULL);

    wl_list_insert(&server->shm_pools, &pool->link);
    
    SERVER_DEBUG("SHM pool created successfully: fd=%d, size=%d, data=%p", 
                fd, size, pool->data);
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
}