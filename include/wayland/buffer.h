#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <wayland-server.h>
#include <drm/drm_fourcc.h>
#include <wayland/shm.h>

enum pixel_format {
    PIXEL_FORMAT_UNKNOWN = 0,
    PIXEL_FORMAT_ARGB8888,
    PIXEL_FORMAT_XRGB8888,
    PIXEL_FORMAT_ABGR8888,
    PIXEL_FORMAT_XBGR8888,
    PIXEL_FORMAT_RGBA8888,
    PIXEL_FORMAT_BGRA8888,
    PIXEL_FORMAT_RGB565,
    PIXEL_FORMAT_BGR565,
    PIXEL_FORMAT_ARGB2101010,
    PIXEL_FORMAT_XRGB2101010,
    PIXEL_FORMAT_ABGR2101010,
    PIXEL_FORMAT_XBGR2101010,
    PIXEL_FORMAT_NV12,
    PIXEL_FORMAT_NV21,
    PIXEL_FORMAT_YUV420,
    PIXEL_FORMAT_YVU420,
};

struct buffer {
    uint32_t width, height;
    struct wl_resource *resource;
    struct wl_list link;

    enum wl_buffer_type {
        WL_BUFFER_SHM,
        WL_BUFFER_DMA_BUF,
        WL_BUFFER_EGL
    } type;

    union {
        struct {
            void *data;
            uint32_t stride;
            // uint32_t format;
        } shm;
        struct {
            int fd;
            // uint32_t format;      // (DRM_FORMAT_*)
            uint64_t modifier;    // format modifier
            uint32_t stride;
            uint32_t offset;
            // uint32_t num_planes;
        } dmabuf;
    };

    enum pixel_format format; 
    size_t size;
};

enum pixel_format wl_shm_format_to_pixel_format(uint32_t wl_format);
enum pixel_format drm_format_to_pixel_format(uint32_t drm_format);

struct buffer *buffer_create_shm(struct wl_resource *resource, void *pool_data, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format);
struct buffer *buffer_create_dmabuf(int fd, uint32_t width, uint32_t height, uint32_t drm_format, uint64_t modifier, uint32_t stride);

#endif