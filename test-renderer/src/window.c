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

void window_mainloop(draw_frame_callback_t draw_callback, vk_wait_idle_t vk_wait_idle) {
    // Убедитесь, что окно создано
    if (!g_window) {
        fprintf(stderr, "GLFW window not initialized!\n");
        return;
    }
    
    printf("Main loop started\n");
    printf("Window size: %dx%d\n", 800, 600);
    
    int frame_counter = 0;
    
    while(!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
        
        // Обработка событий клавиатуры для закрытия окна
        if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(g_window, GLFW_TRUE);
        }
        
        // Рисуем кадр
        draw_callback();
        
        frame_counter++;
        if (frame_counter % 60 == 0) {
            printf("Frame %d\n", frame_counter);
        }
        
        // Небольшая задержка для CPU
        glfwWaitEventsTimeout(0.001);
    }

    vk_wait_idle();
    printf("Main loop finished\n");
}

void create_surface(VkInstance vk_instance, VkSurfaceKHR *vk_surface) {
    if (glfwCreateWindowSurface(vk_instance, g_window, NULL, vk_surface) != VK_SUCCESS) {
        printf("Failed to create window surface!\n");
        exit(5);
    }
}