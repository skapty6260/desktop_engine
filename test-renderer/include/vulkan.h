#pragma once

#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct ShaderFile {
  size_t size;
  char *code;
} ShaderFile;

typedef struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32_t formatCount;
  VkSurfaceFormatKHR *formats;
  uint32_t presentModeCount;
  VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

typedef struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  bool isGraphicsFamilySet;
  uint32_t presentFamily;
  bool isPresentFamilySet;
} QueueFamilyIndices;

struct vulkan {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice gpu;

    VkSwapchainKHR swapchain;
    uint32_t swapchain_image_count;
    VkImage *swapchain_images;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    VkFramebuffer *swapchain_framebuffers;
    VkImageView *swapchain_image_views;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *in_flight_fences;
    VkDescriptorSetLayout descriptor_set_layout;

    QueueFamilyIndices queue_family_indices;
    VkQueue graphics_queue;
    VkQueue present_queue;

    bool validate;
    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    char *extension_names[64];
    char *enabled_layers[64];
};

extern struct vulkan *g_vulkan;

void init_vulkan(bool validate_arg);
void cleanup_vulkan();
void draw_frame(struct vulkan *vulkan);
void readFile(const char *filename, ShaderFile *shader);
VkShaderModule create_shader_module(struct vulkan *vulkan, ShaderFile *shaderFile);
void device_idle();