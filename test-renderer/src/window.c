#include <window.h>
#include <stdio.h>
#include <stdlib.h>

GLFWwindow* g_window = NULL;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void init_window(int width, int height, const char* title) {
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    g_window = glfwCreateWindow(width, height, title, 0, 0);

    printf("Window created successfully\n");
}

void cleanup_window() {
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = NULL;
    }
    glfwTerminate();
}


// TODO: Интегрировать обработку буфера
// По хорошему создать отдельный поток который будет принимать обновления буфера, а тут в каждом кадре проверять буфер на обновление через dirty
void window_mainloop(draw_frame_callback_t draw_callback, vk_wait_idle_t vk_wait_idle) {
    while(!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
        draw_callback();
    }

    vk_wait_idle();
}

void create_surface(VkInstance vk_instance, VkSurfaceKHR *vk_surface) {
    if (glfwCreateWindowSurface(vk_instance, g_window, NULL, vk_surface) != VK_SUCCESS) {
        printf("Failed to create window surface!\n");
        exit(5);
    }
}