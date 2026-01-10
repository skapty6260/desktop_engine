#include <window.h>
#include <vulkan.h>
#include <buffer_mgr.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void draw_callback(void) {
    if (g_buffer_mgr && g_buffer_mgr->buffers && g_buffer_mgr->buffers->dirty) {
        RenderBuffer_t *buffer = g_buffer_mgr->buffers;
        
        printf("New buffer available: %ux%u\n", buffer->width, buffer->height);
        
        // Обновляем текстуру Vulkan
        update_vulkan_texture_from_buffer(g_vulkan, buffer);
        
        // Сбрасываем флаг
        buffer->dirty = false;
    }

    draw_frame(g_vulkan);
}

// static bool parse_args(int argc, char **argv) {
//     for (int i = 1; i < argc; i++) {
//         if (strcmp(argv[i], "--validate") == 0 && i + 1 < argc) {
//             return false;
//         } else return true;
//     }
// }

int main(int argc, char **argv) {
    // bool validate_vulkan = parse_args(argc, argv);

    init_window(800, 600, "Test Renderer");
    create_buffermgr_thread();
    init_vulkan(true);
    window_mainloop(draw_callback, device_idle);
    device_idle();
    stop_buffermgr_thread();    
    cleanup_vulkan();
    cleanup_buffermgr();
    cleanup_window(); 

    return 0;
}