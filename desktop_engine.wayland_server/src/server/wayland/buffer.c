#include <server/wayland/buffer.h>
#include <stdlib.h>

enum pixel_format wl_shm_format_to_pixel_format(uint32_t wl_format) {
    switch (wl_format) {
        case WL_SHM_FORMAT_ARGB8888: return PIXEL_FORMAT_ARGB8888;
        case WL_SHM_FORMAT_XRGB8888: return PIXEL_FORMAT_XRGB8888;
        case WL_SHM_FORMAT_ABGR8888: return PIXEL_FORMAT_ABGR8888;
        case WL_SHM_FORMAT_XBGR8888: return PIXEL_FORMAT_XBGR8888;
        case WL_SHM_FORMAT_RGBA8888: return PIXEL_FORMAT_RGBA8888;
        case WL_SHM_FORMAT_BGRA8888: return PIXEL_FORMAT_BGRA8888;
        case WL_SHM_FORMAT_RGB565:   return PIXEL_FORMAT_RGB565;
        case WL_SHM_FORMAT_BGR565:   return PIXEL_FORMAT_BGR565;
        case WL_SHM_FORMAT_ARGB2101010: return PIXEL_FORMAT_ARGB2101010;
        case WL_SHM_FORMAT_XRGB2101010: return PIXEL_FORMAT_XRGB2101010;
        case WL_SHM_FORMAT_ABGR2101010: return PIXEL_FORMAT_ABGR2101010;
        case WL_SHM_FORMAT_XBGR2101010: return PIXEL_FORMAT_XBGR2101010;
        default: return PIXEL_FORMAT_UNKNOWN;
    }
}

enum pixel_format drm_format_to_pixel_format(uint32_t drm_format) {
    switch (drm_format) {
        case DRM_FORMAT_ARGB8888: return PIXEL_FORMAT_ARGB8888;
        case DRM_FORMAT_XRGB8888: return PIXEL_FORMAT_XRGB8888;
        case DRM_FORMAT_ABGR8888: return PIXEL_FORMAT_ABGR8888;
        case DRM_FORMAT_XBGR8888: return PIXEL_FORMAT_XBGR8888;
        case DRM_FORMAT_RGBA8888: return PIXEL_FORMAT_RGBA8888;
        case DRM_FORMAT_BGRA8888: return PIXEL_FORMAT_BGRA8888;
        case DRM_FORMAT_RGB565:   return PIXEL_FORMAT_RGB565;
        case DRM_FORMAT_BGR565:   return PIXEL_FORMAT_BGR565;
        case DRM_FORMAT_ARGB2101010: return PIXEL_FORMAT_ARGB2101010;
        case DRM_FORMAT_XRGB2101010: return PIXEL_FORMAT_XRGB2101010;
        case DRM_FORMAT_ABGR2101010: return PIXEL_FORMAT_ABGR2101010;
        case DRM_FORMAT_XBGR2101010: return PIXEL_FORMAT_XBGR2101010;
        case DRM_FORMAT_NV12:     return PIXEL_FORMAT_NV12;
        case DRM_FORMAT_NV21:     return PIXEL_FORMAT_NV21;
        case DRM_FORMAT_YUV420:   return PIXEL_FORMAT_YUV420;
        case DRM_FORMAT_YVU420:   return PIXEL_FORMAT_YVU420;
        default: return PIXEL_FORMAT_UNKNOWN;
    }
}

struct buffer *buffer_create_shm(struct wl_resource *resource, void *pool_data, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) {
    struct buffer *buf = calloc(1, sizeof(struct buffer));
    if (!buf) return NULL;
    
    buf->type = WL_BUFFER_SHM;
    buf->width = width;
    buf->height = height;
    buf->resource = resource;
    
    buf->shm.stride = stride;
    
    if (pool_data) {
        buf->shm.data = (unsigned char*)pool_data + offset;
    } else {
        buf->shm.data = NULL;
    }
    
    buf->size = buf->shm.stride * buf->height;
    buf->format = wl_shm_format_to_pixel_format(wl_format);
    
    SERVER_DEBUG("Created buffer from SHM: %dx%d, stride=%d, format=0x%x, data=%p",
                buf->width, buf->height, buf->shm.stride, buf->format, buf->shm.data);
    
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