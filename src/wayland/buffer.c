#include <wayland/buffer.h>
#include <logger.h>
#include <stdlib.h>

enum pixel_format wl_shm_format_to_pixel_format(uint32_t wl_format) {
    switch (wl_format) {
        case WL_SHM_FORMAT_ARGB8888: return PIXEL_FORMAT_ARGB8888;
        case WL_SHM_FORMAT_XRGB8888: return PIXEL_FORMAT_XRGB8888;
        default: return PIXEL_FORMAT_UNKNOWN;
    }
}

enum pixel_format drm_format_to_pixel_format(uint32_t drm_format) {
    switch (drm_format) {
        case DRM_FORMAT_ARGB8888: return PIXEL_FORMAT_ARGB8888;
        case DRM_FORMAT_XRGB8888: return PIXEL_FORMAT_XRGB8888;
        default: return PIXEL_FORMAT_UNKNOWN;
    }
}

struct buffer *buffer_create_shm(struct wl_resource *resource, 
                                 uint32_t id, int32_t offset, int32_t width, 
                                 int32_t height, int32_t stride, uint32_t format,
                                 int fd) {  // Добавляем fd параметр
    struct buffer *buf = calloc(1, sizeof(struct buffer));
    if (!buf) {
        SERVER_ERROR("Failed to allocate buffer");
        return NULL;
    }
    
    buf->type = WL_BUFFER_SHM;
    buf->width = width;
    buf->height = height;
    buf->resource = resource;
    buf->shm.stride = stride;
    buf->shm.fd = fd;
    
    buf->size = buf->shm.stride * buf->height;
    buf->format = wl_shm_format_to_pixel_format(format);
    
    SERVER_DEBUG("SHM buffer created: %dx%d, stride=%d, fd=%d, data=%p, size=%zu", 
                width, height, stride, fd, buf->shm.data, buf->size);
    
    return buf;
}

struct buffer *buffer_create_dmabuf(int fd, uint32_t width, uint32_t height, uint32_t drm_format, uint64_t modifier, uint32_t stride) {
    struct buffer *buf = calloc(1, sizeof(struct buffer));
    if (!buf) return NULL;
    
    buf->type = WL_BUFFER_DMA_BUF;
    buf->width = width;
    buf->height = height;
    buf->dmabuf.fd = fd;
    buf->dmabuf.modifier = modifier;
    buf->dmabuf.stride = stride;
    buf->size = stride * height;
    buf->format = drm_format_to_pixel_format(drm_format);
    
    return buf;
}