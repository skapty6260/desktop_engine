#include <wayland/compositor.h>
#include <wayland/server.h>
#include <wayland/buffer.h>
#include <logger.h>
#include <stdlib.h>
#include <dbus-server/modules/buffer_module.h>
#include <dbus-server/server.h>

#ifdef HAVE_LINUX_DMABUF
#include "linux-dmabuf-unstable-v1-protocol.h"
#endif

static struct buffer *custom_buffer_from_resource(struct wl_resource *resource) {
    struct buffer *buffer = calloc(1, sizeof(struct buffer));
    if (!buffer) {
        SERVER_ERROR("Failed to allocate memory for buffer");
        return NULL;
    }
    
    struct buffer *custom_shm_buffer = wl_resource_get_user_data(resource);
    if (custom_shm_buffer) {
        // Это наш ручной SHM буфер
        buffer->type = WL_BUFFER_SHM;
        buffer->resource = resource;
        buffer->width = custom_shm_buffer->width;
        buffer->height = custom_shm_buffer->height;
        
        return buffer;
    }

#ifdef HAVE_LINUX_DMABUF
    // Проверяем DMA-BUF через linux-dmabuf extension
    if (wl_resource_instance_of(resource, &zwp_linux_buffer_params_v1_interface, NULL)) {
        buffer->type = WL_BUFFER_DMA_BUF;
        // Получаем параметры DMA-BUF
        SERVER_DEBUG("Buffer is dmabuf!");
        return buffer;
    }
#endif

    // Если не SHM и не DMA-BUF, освобождаем память
    free(buffer);
    return NULL;
}

static const char* buffer_type_to_string(struct buffer *buffer) {
    if (!buffer) return "NULL";
    
    switch (buffer->type) {
        case WL_BUFFER_SHM:
            return "SHM";
        case WL_BUFFER_EGL:
            return "EGL";
        case WL_BUFFER_DMA_BUF:
            return "DMA-BUF";
        default:
            return "UNKNOWN";
    }
}

static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);
    
    SERVER_DEBUG("SURFACE: Resource destroyed, surface=%p", surface);
    
    if (surface) {
        wl_list_remove(&surface->link);
        free(surface);
    }
}

static void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    SERVER_DEBUG("SURFACE DAMAGE CALLED");
}

static void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
    SERVER_DEBUG("SURFACE FRAME CALLED");
}

static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    SERVER_DEBUG("SURFACE SET_OPAQUE_REGION CALLED");
}

static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    SERVER_DEBUG("SURFACE SET_INPUT_REGION CALLED");
}

static void surface_commit(struct wl_client *client, struct wl_resource *resource) {
    // struct surface *surface = wl_resource_get_user_data(resource);

    // if (!surface) return;
    
    // if (surface->pending_changes.attach == true) {
    //     if (surface->buffer) {
    //         wl_buffer_send_release(surface->buffer);
    //     }

    //     // Set new buffer (May be NULL to detach)
    //     if (surface->pending_buffer) {
    //         surface->buffer = surface->pending_buffer;
    //         surface->pending_buffer = NULL; // Передача владения

    //         // Обновляем размеры поверхности (Should add get buffer size)
    //         int width, height;
    //         // if (get_buffer_size(surface->buffer, &width, &height)) {
    //         //     // Применяем масштабирование
    //         //     surface->width = width / surface->buffer_scale;
    //         //     surface->height = height / surface->buffer_scale;
    //         // }
    //         SERVER_DEBUG("Surface committed: buffer=%p, size=%dx%d", surface->buffer, surface->width, surface->height);
    //     } else {
    //         // Buffer detach
    //         surface->width = 0;
    //         surface->height = 0;
    //         SERVER_DEBUG("Surface committed: buffer detached");
    //     }

    //     surface->pending_changes.attach = false;
    // }
    SERVER_DEBUG("SURFACE COMMIT CALLED");
}

static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform) {
    SERVER_DEBUG("SURFACE SET_BUFFER_TRANSFORM CALLED");
}

static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale) {
    SERVER_DEBUG("SURFACE SET_BUFFER_SCALE CALLED");
}

static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    SERVER_DEBUG("SURFACE DAMAGE BUFFER CALLED");
}

static void surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {
    SERVER_DEBUG("SURFACE OFFSET CALLED");
}

static void surface_headless_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y) {
    SERVER_DEBUG(">>>>>> ENTERING surface_headless_attach <<<<<<");
    SERVER_DEBUG("resource=%p, buffer=%p, x=%d, y=%d", resource, buffer_resource, x, y);
    
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface || !buffer_resource) return;
    
    struct buffer *buffer = wl_resource_get_user_data(buffer_resource);
    SERVER_DEBUG("Called attach buffer with type: %s, size: %i or %iX%i\nBuffer data pointer: %p", 
                 buffer_type_to_string(buffer), buffer->size, buffer->width, buffer->height, buffer->shm.data);

    // Отправляем D-Bus сигнал о новом буфере
    if (surface->server && surface->server->dbus_server) {
        // Получаем D-Bus соединение
        DBusConnection *conn = surface->server->dbus_server->connection;
        
        if (conn) {
            // Создаем BufferInfo
            BufferInfo info = {
                .width = buffer->width,
                .height = buffer->height,
                .stride = buffer->width * 4,  // Предполагаем 32 бита на пиксель
                .format = buffer->type == WL_BUFFER_SHM ? WL_SHM_FORMAT_XRGB8888 : 0,
                .format_str = buffer_type_to_string(buffer),
                .size = buffer->size,
                .has_data = (buffer->shm.data != NULL),
                .type = buffer->type
            };
            
            // Отправляем сигнал
            buffer_module_send_update_signal(conn, &info);
            SERVER_DEBUG("D-Bus update signal sent for buffer %dx%d", buffer->width, buffer->height);
        } else {
            SERVER_DEBUG("No D-Bus connection available");
        }
    } else {
        SERVER_DEBUG("No D-Bus server available for sending buffer update");
    }
}

const struct wl_surface_interface surface_implementation = {
    .destroy = surface_destroy,
    .attach = surface_headless_attach,
    .damage = surface_damage,
    .frame = surface_frame,
    .set_opaque_region = surface_set_opaque_region,
    .set_input_region = surface_set_input_region,
    .commit = surface_commit,
    .set_buffer_transform = surface_set_buffer_transform,
    .set_buffer_scale = surface_set_buffer_scale,
    .damage_buffer = surface_damage_buffer,
    .offset = surface_offset,
};