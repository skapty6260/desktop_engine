#include <window.h>
#include <vulkan.h>
#include <buffer_fetcher.h>
#include <stdio.h>

void draw_callback(void) {
    draw_frame(g_vulkan);
}

int main() {
    init_window(800, 600, "Test Renderer");
    buffer_fetcher_init();
    init_vulkan();
    window_mainloop(draw_callback, device_idle);
    buffer_fetcher_cleanup();
    cleanup_vulkan();
    cleanup_window();

    printf("Renderer finished and cleaned up");

    return 0;
}