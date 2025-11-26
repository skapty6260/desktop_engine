#include "compositor.h"
#include "server.h"
#include "../logger/logger.h"
#include <stdlib.h>

#ifdef HAVE_LINUX_DMABUF
#include "linux-dmabuf-unstable-v1-protocol.h"
#endif


/*  WL_SURFACE DESCRIPTION
    A surface is a rectangular area that may be displayed on zero or more outputs,
    and shown any number of times at the compositor's discretion. It can present
    wl_buffers, receive user input, and define a local coordinate system.

    Surface size and relative positions are described in surface-local coordinates,
    which may differ from buffer pixel coordinates when buffer_transform or
    buffer_scale is used.

    A surface without a role is useless - the compositor doesn't know where, when
    or how to present it. The role defines the surface's purpose. 
    Role Examples include:
    - a pointer cursor (set by wl_pointer.set_cursor),
    - a drag icon (wl_data_device.start_drag),
    - a sub-surface (wl_subcompositor.get_subsurface),
    - a window via shell protocols (XDG).

    Key rules:
    - Surface can have only one role at a time
    - Initially surface has no role
    - Once set, role is permanent for the surface's lifetime
    - Re-setting the same role is allowed unless explicitly forbidden
    - Role objects (like wl_subsurface) must be destroyed before the surface
    - Destroying role object stops the role execution but doesn't remove the role
    - Role switching between different types is prohibited
*/

/*  WL_SURFACE attach
*/ 
/*
Устанавливает буфер в качестве содержимого поверхности.

Новый размер поверхности вычисляется на основе размера буфера с применением
обратного buffer_transform и обратного buffer_scale. Это означает, что при
коммите размер буфера должен быть целым кратным buffer_scale. Если это не так,
отправляется ошибка invalid_size.

Аргументы x и y задают расположение верхнего левого угла нового отложенного буфера
относительно верхнего левого угла текущего буфера в поверхностно-локальных координатах.
Другими словами, x и y вместе с новым размером поверхности определяют, в каких
направлениях изменяется размер поверхности. Использование значений x и y, отличных
от 0, не рекомендуется и должно быть заменено использованием отдельного запроса
wl_surface.offset.

Когда связанная версия wl_surface равна 5 или выше, передача любых не-null значений
x или y является нарушением протокола и приведет к ошибке 'invalid_offset'.
Аргументы x и y игнорируются и не изменяют отложенное состояние. Для достижения
эквивалентной семантики используйте wl_surface.offset.

Содержимое поверхности имеет двойную буферизацию, см. wl_surface.commit.

Начальное содержимое поверхности пустое. wl_surface.attach назначает заданный
wl_buffer в качестве отложенного wl_buffer. wl_surface.commit делает отложенный
wl_buffer новым содержимым поверхности, и размер поверхности становится равным
размеру, вычисленному из wl_buffer, как описано выше. После коммита отложенного
буфера нет до следующего attach.

Коммит отложенного wl_buffer позволяет композитору читать пиксели в wl_buffer.
Композитор может получить доступ к пикселям в любое время после запроса
wl_surface.commit. Когда композитор больше не будет обращаться к пикселям,
он отправит событие wl_buffer.release. Только после получения wl_buffer.release
клиент может повторно использовать wl_buffer. wl_buffer, который был attached,
а затем заменен другим attach вместо коммита, не получит событие release и не
используется композитором.

Если отложенный wl_buffer был закоммичен на несколько wl_surface, доставка
событий wl_buffer.release становится неопределенной. Корректный клиент не должен
полагаться на события wl_buffer.release в этом случае. Альтернативно, клиент
может создать несколько объектов wl_buffer из одного хранилища или использовать
расширение протокола, предоставляющее уведомления о release при каждом коммите.

Уничтожение wl_buffer после wl_buffer.release не изменяет содержимое поверхности.
Уничтожение wl_buffer до wl_buffer.release разрешено, пока базовое хранилище
буфера не используется повторно (это может произойти, например, при завершении
процесса клиента). Однако, если клиент уничтожает wl_buffer до получения события
wl_buffer.release и изменяет базовое хранилище буфера, содержимое поверхности
немедленно становится неопределенным.

Если wl_surface.attach отправляется с NULL wl_buffer, следующий wl_surface.commit
удалит содержимое поверхности.

Если отложенный wl_buffer был уничтожен, результат не определен. Известно, что
многие композиторы удаляют содержимое поверхности при следующем wl_surface.commit,
но это поведение не является универсальным. Клиенты, стремящиеся максимизировать
совместимость, не должны уничтожать отложенные буферы и должны гарантировать, что
они явно удаляют содержимое с поверхностей, даже после уничтожения буферов.
*/
static void surface_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y) {
    struct surface *surface = wl_resource_get_user_data(resource);
    const uint32_t surface_version = wl_resource_get_version(resource);

    if (!surface) return;

    if (surface_version >= 5 && (x != 0 || y != 0)) {
        wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_OFFSET,
                              "x and y must be 0 for wl_surface version >= 5");
        return;
    }

    // Should add get_buffer_size and check for invalid_size and invalid_offset error

    surface->pending_buffer = buffer;
    surface->pending_x = (surface_version >= 5) ? 0 : x;
    surface->pending_y = (surface_version >= 5) ? 0 : y;
    
    surface->pending_changes.attach = true;

    SERVER_DEBUG("SURFACE ATTACH: surface=%p, pending_buffer=%p, pending_x=%d, pending_y=%d", surface, surface->pending_buffer, surface->pending_x, surface->pending_y);
}

static struct buffer *custom_buffer_from_resource(struct wl_resource *resource) {
    struct buffer *buffer = calloc(1, sizeof(struct buffer));
    if (!buffer) {
        SERVER_ERROR("Failed to allocate memory for buffer");
        return NULL;
    }
    
    // Проверяем SHM буфер
    SERVER_DEBUG("Checking for SHM buffer...");
    struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(resource);
    SERVER_DEBUG("wl_shm_buffer_get() returned: %p", (void*)shm_buffer);

    if (shm_buffer) {
        buffer->type = WL_BUFFER_SHM;
        buffer->resource = resource;
        buffer->width = wl_shm_buffer_get_width(shm_buffer);
        buffer->height = wl_shm_buffer_get_height(shm_buffer);
        buffer->shm.shm_buffer = shm_buffer;

        SERVER_DEBUG("SHM buffer detected: %dx%d", buffer->width, buffer->height);
        SERVER_DEBUG("SHM format: 0x%x", wl_shm_buffer_get_format(shm_buffer));
        SERVER_DEBUG("SHM stride: %d", wl_shm_buffer_get_stride(shm_buffer));
        return buffer;
    }

#ifdef HAVE_LINUX_DMABUF
    // Проверяем DMA-BUF через linux-dmabuf extension
    if (wl_resource_instance_of(resource, &zwp_linux_buffer_params_v1_interface, NULL)) {
        buffer->type = WL_BUFFER_DMA_BUF;
        // Получаем параметры DMA-BUF
        // get_dmabuf_params(buffer, resource);
        SERVER_DEBUG("Buffer is dmabuf!");
        return buffer;
    }
#endif

    return NULL;
}

static const char* buffer_type_to_string(struct buffer *buffer) {
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

static void surface_headless_attach(struct wl_client *client, 
                                   struct wl_resource *resource, 
                                   struct wl_resource *buffer_resource, 
                                   int32_t x, int32_t y) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface || !buffer_resource) return;
    
    uint32_t buffer_id = wl_resource_get_id(buffer_resource);
    uint32_t version = wl_resource_get_version(buffer_resource);
    
    SERVER_INFO("Buffer attached - ID: %d, Version: %d", buffer_id, version);
    
    struct buffer *buffer = custom_buffer_from_resource(buffer_resource);
    if (buffer) {
        SERVER_DEBUG("Buffer attached headless, buffer type: %s", buffer_type_to_string(buffer));
        SERVER_DEBUG("Buffer not attached (It's type not defined)");
    }
}

/*  WL_SURFACE damage
*/ 
static void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE DAMAGE CALLED");
}

/*  WL_SURFACE frame
*/ 
static void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE FRAME CALLED");
}

/*  WL_SURFACE set_opaque_region
*/ 
static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_OPAQUE_REGION CALLED");
}

/*  WL_SURFACE set_input_region 
*/ 
static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_INPUT_REGION CALLED");
}

/*  WL_SURFACE commit
*/ 
static void surface_commit(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) return;
    
    if (surface->pending_changes.attach == true) {
        if (surface->buffer) {
            wl_buffer_send_release(surface->buffer);
        }

        // Set new buffer (May be NULL to detach)
        if (surface->pending_buffer) {
            surface->buffer = surface->pending_buffer;
            surface->pending_buffer = NULL; // Передача владения

            // Обновляем размеры поверхности (Should add get buffer size)
            int width, height;
            // if (get_buffer_size(surface->buffer, &width, &height)) {
            //     // Применяем масштабирование
            //     surface->width = width / surface->buffer_scale;
            //     surface->height = height / surface->buffer_scale;
            // }
            SERVER_DEBUG("Surface committed: buffer=%p, size=%dx%d", surface->buffer, surface->width, surface->height);
        } else {
            // Buffer detach
            surface->width = 0;
            surface->height = 0;
            SERVER_DEBUG("Surface committed: buffer detached");
        }

        surface->pending_changes.attach = false;
    }
}

/*  WL_SURFACE set_buffer_transform
*/ 
static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }
    SERVER_DEBUG("SURFACE SET_BUFFER_TRANSFORM CALLED");
}

/*  WL_SURFACE set_buffer_scale
*/ 
static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE SET_BUFFER_SCALE CALLED");
}

/*  WL_SURFACE damage_buffer
*/ 
static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE DAMAGE BUFFER CALLED");
}

/*  WL_SURFACE offset
*/ 
static void surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {
    struct surface *surface = wl_resource_get_user_data(resource);

    if (!surface) {
        return;
    }

    SERVER_DEBUG("SURFACE OFFSET CALLED");
}

/*  WL_SURFACE destroy
*/
static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    struct surface *surface = wl_resource_get_user_data(resource);
    
    SERVER_DEBUG("SURFACE: Resource destroyed, surface=%p", surface);
    
    if (surface) {
        wl_list_remove(&surface->link);
        free(surface);
    }
}

/*  WL_SURFACE IMPLEMENTATION
    REQUEST destroy()
    REQUEST attach(buffer: object<wl_buffer>, x: int, y: int)
        Argument        Type           Description
        buffer	object<wl_buffer>	   buffer of surface contents
        x	    int	                   surface-local x coordinate
        y	    int	                   surface-local y coordinate
    REQUEST damage(x: int, y: int, width: int, height: int)
        Argument        Type        Description
        x	            int	        surface-local x coordinate
        y	            int	        surface-local y coordinate
        width	        int	        width of damage rectangle
        height	        int	        height of damage rectangle
    REQUEST frame(callback: new_id<wl_callback>)
        Argument            Type            Description
        callback	new_id<wl_callback>	    callback object for the frame request
    REQUEST set_opaque_region(region: object<wl_region>)
        Argument            Type           Description
        region	    object<wl_region>	   opaque region of the surface
    REQUEST set_input_region(region: object<wl_region>)
        Argument            Type           Description
        region	    object<wl_region>	   input region of the surface
    REQUEST commit()
    REQUEST set_buffer_transform(transform: int<wl_output.transform>)
        Argument            Type                Description
        transform	int<wl_output.transform>	transform for interpreting buffer contents
    REQUEST set_buffer_scale(scale: int)
        Argument        Type        Description
        scale	        int	        scale for interpreting buffer contents
    REQUEST damage_buffer(x: int, y: int, width: int, height: int)
        Argument        Type        Description
        x	            int	        bufer-local x coordinate
        y	            int	        bufer-local y coordinate
        width	        int	        width of damage rectangle
        height	        int	        height of damage rectangle
    REQUEST offset(x: int, y: int)
        Argument        Type        Description
        x	            int	        surface-local x coordinate
        y	            int	        surface-local y coordinate
*/
static const struct wl_surface_interface surface_implementation = {
    .destroy = surface_destroy,
    .attach = surface_headless_attach, // surface_attach
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

/*  WL_COMPOSITOR DESCRIPTION
    A compositor. This object is a singleton global.
    The compositor is in charge of combining the contents of multiple surfaces
    into one displayable output.
*/


//  WL_COMPOSITOR create_surface
static void compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct server *server = wl_resource_get_user_data(resource);
    
    struct wl_resource *surface_resource = wl_resource_create(
        client, &wl_surface_interface, wl_resource_get_version(resource), id);
    
    if (!surface_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    // Создаем структуру поверхности
    struct surface *surface = calloc(1, sizeof(struct surface));
    if (!surface) {
        wl_client_post_no_memory(client);
        wl_resource_destroy(surface_resource);
        return;
    }
    
    surface->resource = surface_resource;
    surface->buffer = NULL;
    surface->pending_buffer = NULL;
    surface->server = server;
    surface->xdg_surface = NULL;
    surface->xdg_toplevel = NULL;
    surface->x = 0;
    surface->y = 0;
    surface->pending_x = 0;
    surface->pending_y = 0;
    surface->pending_width = 0;
    surface->pending_height = 0;
    surface->width = 0;
    surface->height = 0;
    wl_list_init(&surface->link);
    
    wl_resource_set_implementation(surface_resource, &surface_implementation, surface, NULL); // TODO: destructor
    
    wl_list_insert(&server->surfaces, &surface->link);

    SERVER_DEBUG("COMPOSITOR: Surface created successfully, resource=%p, implementation set, resource version: %i", surface_resource, wl_resource_get_version(surface_resource));
}

//  WL_COMPOSITOR create_region
static void compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct wl_resource *region_resource = wl_resource_create(
        client, &wl_region_interface, 1, id);
    
    if (!region_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    SERVER_DEBUG("Region created");
}

/*  WL_COMPOSITOR IMPLEMENTATION
    REQUEST create_surface(id: new_id<wl_surface>)
        Argument            Type            Description
        id	        new_id<wl_surface>	    the new surface  
    REQUEST create_region(id: new_id<wl_region>)
        Argument        Type                Description
        id	        new_id<wl_region>	    the new region
*/
static const struct wl_compositor_interface compositor_implementation = {
    .create_surface = compositor_create_surface,
    .create_region = compositor_create_region,
};

void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id) {    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_compositor_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    wl_resource_set_implementation(resource, &compositor_implementation, data, NULL);
    
    SERVER_DEBUG("COMPOSITOR: Binding to client, requested version=%u, id=%u", 
                 version, id);
}