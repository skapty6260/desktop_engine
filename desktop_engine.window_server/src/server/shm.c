#include "server.h"
#include "logger/logger.h"
#include "shm.h"

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