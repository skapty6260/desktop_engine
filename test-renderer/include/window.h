#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

extern GLFWwindow* g_window;

typedef void (*draw_frame_callback_t)(void);
typedef void (*vk_wait_idle_t)(void);

void init_window(int width, int height, const char* title);
void cleanup_window();
void window_mainloop(draw_frame_callback_t draw_callback, vk_wait_idle_t vk_wait_idle);
void create_surface(VkInstance vk_instance, VkSurfaceKHR *vk_surface);