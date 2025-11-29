// #include "compositor.h"
// #include "server.h"
// #include "../logger/logger.h"
// #include <stdlib.h>

// #ifdef HAVE_LINUX_DMABUF
// #include "linux-dmabuf-unstable-v1-protocol.h"
// #endif


// /* TODO
// Rework compositor to do nothing, but attach client surface buffer to custom buffer
// and transfer it to renderer.

// Main functionality: 
// bind compositor;
// implement compositor methods; 
// handle surface constructor (surface_create) and destructor;
// surface_attach (bind buffer);
// surface_destroy (destructor);

// All other methods should be dummies to be compatible for all clients
// */

// static struct buffer *custom_buffer_from_resource(struct wl_resource *resource) {
//     struct buffer *buffer = calloc(1, sizeof(struct buffer));
//     if (!buffer) {
//         SERVER_ERROR("Failed to allocate memory for buffer");
//         return NULL;
//     }
    
//     struct buffer *custom_shm_buffer = wl_resource_get_user_data(resource);
//     if (custom_shm_buffer) {
//         // Это наш ручной SHM буфер
//         buffer->type = WL_BUFFER_SHM;
//         buffer->resource = resource;
//         buffer->width = custom_shm_buffer->width;
//         buffer->height = custom_shm_buffer->height;
        
//         SERVER_DEBUG("Custom SHM buffer: %dx%d",
//                     buffer->width, buffer->height);
//         return buffer;
//     }

// #ifdef HAVE_LINUX_DMABUF
//     // Проверяем DMA-BUF через linux-dmabuf extension
//     if (wl_resource_instance_of(resource, &zwp_linux_buffer_params_v1_interface, NULL)) {
//         buffer->type = WL_BUFFER_DMA_BUF;
//         // Получаем параметры DMA-BUF
//         SERVER_DEBUG("Buffer is dmabuf!");
//         return buffer;
//     }
// #endif

//     // Если не SHM и не DMA-BUF, освобождаем память
//     free(buffer);
//     return NULL;
// }

// static const char* buffer_type_to_string(struct buffer *buffer) {
//     switch (buffer->type) {
//         case WL_BUFFER_SHM:
//         return "SHM";

//         case WL_BUFFER_EGL:
//         return "EGL";

//         case WL_BUFFER_DMA_BUF:
//         return "DMA-BUF";

//         default:
//         return "UNKNOWN";
//     }
// }

// static void surface_headless_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface || !buffer_resource) return;
    
//     struct buffer *buffer = custom_buffer_from_resource(buffer_resource);
//     if (buffer) {
//         SERVER_DEBUG("Buffer attached headless, buffer type: %s", buffer_type_to_string(buffer));
//     } else {
//         SERVER_DEBUG("Buffer not attached (It's type not defined)");
//     }
// }

// /*  WL_SURFACE damage
// */ 
// static void surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }
//     SERVER_DEBUG("SURFACE DAMAGE CALLED");
// }

// /*  WL_SURFACE frame
// */ 
// static void surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }
//     SERVER_DEBUG("SURFACE FRAME CALLED");
// }

// /*  WL_SURFACE set_opaque_region
// */ 
// static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }
//     SERVER_DEBUG("SURFACE SET_OPAQUE_REGION CALLED");
// }

// /*  WL_SURFACE set_input_region 
// */ 
// static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }
//     SERVER_DEBUG("SURFACE SET_INPUT_REGION CALLED");
// }

// /*  WL_SURFACE commit
// */ 
// static void surface_commit(struct wl_client *client, struct wl_resource *resource) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) return;
    
//     if (surface->pending_changes.attach == true) {
//         if (surface->buffer) {
//             wl_buffer_send_release(surface->buffer);
//         }

//         // Set new buffer (May be NULL to detach)
//         if (surface->pending_buffer) {
//             surface->buffer = surface->pending_buffer;
//             surface->pending_buffer = NULL; // Передача владения

//             // Обновляем размеры поверхности (Should add get buffer size)
//             int width, height;
//             // if (get_buffer_size(surface->buffer, &width, &height)) {
//             //     // Применяем масштабирование
//             //     surface->width = width / surface->buffer_scale;
//             //     surface->height = height / surface->buffer_scale;
//             // }
//             SERVER_DEBUG("Surface committed: buffer=%p, size=%dx%d", surface->buffer, surface->width, surface->height);
//         } else {
//             // Buffer detach
//             surface->width = 0;
//             surface->height = 0;
//             SERVER_DEBUG("Surface committed: buffer detached");
//         }

//         surface->pending_changes.attach = false;
//     }
// }

// /*  WL_SURFACE set_buffer_transform
// */ 
// static void surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }
//     SERVER_DEBUG("SURFACE SET_BUFFER_TRANSFORM CALLED");
// }

// /*  WL_SURFACE set_buffer_scale
// */ 
// static void surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }

//     SERVER_DEBUG("SURFACE SET_BUFFER_SCALE CALLED");
// }

// /*  WL_SURFACE damage_buffer
// */ 
// static void surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }

//     SERVER_DEBUG("SURFACE DAMAGE BUFFER CALLED");
// }

// /*  WL_SURFACE offset
// */ 
// static void surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {
//     struct surface *surface = wl_resource_get_user_data(resource);

//     if (!surface) {
//         return;
//     }

//     SERVER_DEBUG("SURFACE OFFSET CALLED");
// }

// /*  WL_SURFACE destroy
// */
// static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
//     struct surface *surface = wl_resource_get_user_data(resource);
    
//     SERVER_DEBUG("SURFACE: Resource destroyed, surface=%p", surface);
    
//     if (surface) {
//         wl_list_remove(&surface->link);
//         free(surface);
//     }
// }

// /*  WL_SURFACE IMPLEMENTATION
//     REQUEST destroy()
//     REQUEST attach(buffer: object<wl_buffer>, x: int, y: int)
//         Argument        Type           Description
//         buffer	object<wl_buffer>	   buffer of surface contents
//         x	    int	                   surface-local x coordinate
//         y	    int	                   surface-local y coordinate
//     REQUEST damage(x: int, y: int, width: int, height: int)
//         Argument        Type        Description
//         x	            int	        surface-local x coordinate
//         y	            int	        surface-local y coordinate
//         width	        int	        width of damage rectangle
//         height	        int	        height of damage rectangle
//     REQUEST frame(callback: new_id<wl_callback>)
//         Argument            Type            Description
//         callback	new_id<wl_callback>	    callback object for the frame request
//     REQUEST set_opaque_region(region: object<wl_region>)
//         Argument            Type           Description
//         region	    object<wl_region>	   opaque region of the surface
//     REQUEST set_input_region(region: object<wl_region>)
//         Argument            Type           Description
//         region	    object<wl_region>	   input region of the surface
//     REQUEST commit()
//     REQUEST set_buffer_transform(transform: int<wl_output.transform>)
//         Argument            Type                Description
//         transform	int<wl_output.transform>	transform for interpreting buffer contents
//     REQUEST set_buffer_scale(scale: int)
//         Argument        Type        Description
//         scale	        int	        scale for interpreting buffer contents
//     REQUEST damage_buffer(x: int, y: int, width: int, height: int)
//         Argument        Type        Description
//         x	            int	        bufer-local x coordinate
//         y	            int	        bufer-local y coordinate
//         width	        int	        width of damage rectangle
//         height	        int	        height of damage rectangle
//     REQUEST offset(x: int, y: int)
//         Argument        Type        Description
//         x	            int	        surface-local x coordinate
//         y	            int	        surface-local y coordinate
// */
// static const struct wl_surface_interface surface_implementation = {
//     .destroy = surface_destroy,
//     .attach = surface_headless_attach, // surface_attach
//     .damage = surface_damage,
//     .frame = surface_frame,
//     .set_opaque_region = surface_set_opaque_region,
//     .set_input_region = surface_set_input_region,
//     .commit = surface_commit,
//     .set_buffer_transform = surface_set_buffer_transform,
//     .set_buffer_scale = surface_set_buffer_scale,
//     .damage_buffer = surface_damage_buffer,
//     .offset = surface_offset,
// };

// /*  WL_COMPOSITOR DESCRIPTION
//     A compositor. This object is a singleton global.
//     The compositor is in charge of combining the contents of multiple surfaces
//     into one displayable output.
// */


// //  WL_COMPOSITOR create_surface
// static void compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
//     struct server *server = wl_resource_get_user_data(resource);
    
//     struct wl_resource *surface_resource = wl_resource_create(
//         client, &wl_surface_interface, wl_resource_get_version(resource), id);
    
//     if (!surface_resource) {
//         wl_client_post_no_memory(client);
//         return;
//     }
    
//     // Создаем структуру поверхности
//     struct surface *surface = calloc(1, sizeof(struct surface));
//     if (!surface) {
//         wl_client_post_no_memory(client);
//         wl_resource_destroy(surface_resource);
//         return;
//     }
    
//     surface->resource = surface_resource;
//     surface->buffer = NULL;
//     surface->pending_buffer = NULL;
//     surface->server = server;
//     surface->xdg_surface = NULL;
//     surface->xdg_toplevel = NULL;
//     surface->x = 0;
//     surface->y = 0;
//     surface->pending_x = 0;
//     surface->pending_y = 0;
//     surface->pending_width = 0;
//     surface->pending_height = 0;
//     surface->width = 0;
//     surface->height = 0;
//     wl_list_init(&surface->link);
    
//     wl_resource_set_implementation(surface_resource, &surface_implementation, surface, NULL); // TODO: destructor
    
//     wl_list_insert(&server->surfaces, &surface->link);

//     SERVER_DEBUG("COMPOSITOR: Surface created successfully, resource=%p, implementation set, resource version: %i", surface_resource, wl_resource_get_version(surface_resource));
// }

// //  WL_COMPOSITOR create_region
// static void compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
//     struct wl_resource *region_resource = wl_resource_create(
//         client, &wl_region_interface, 1, id);
    
//     if (!region_resource) {
//         wl_client_post_no_memory(client);
//         return;
//     }
    
//     SERVER_DEBUG("Region created");
// }

// /*  WL_COMPOSITOR IMPLEMENTATION
//     REQUEST create_surface(id: new_id<wl_surface>)
//         Argument            Type            Description
//         id	        new_id<wl_surface>	    the new surface  
//     REQUEST create_region(id: new_id<wl_region>)
//         Argument        Type                Description
//         id	        new_id<wl_region>	    the new region
// */
// static const struct wl_compositor_interface compositor_implementation = {
//     .create_surface = compositor_create_surface,
//     .create_region = compositor_create_region,
// };

// void bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id) {    
//     struct wl_resource *resource = wl_resource_create(
//         client, &wl_compositor_interface, version, id);
    
//     if (!resource) {
//         wl_client_post_no_memory(client);
//         return;
//     }
    
//     wl_resource_set_implementation(resource, &compositor_implementation, data, NULL);
    
//     SERVER_DEBUG("COMPOSITOR: Binding to client, requested version=%u, id=%u", 
//                  version, id);
// }