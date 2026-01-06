#include <vulkan.h>
#include <window.h>
#include <macro.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

struct vulkan *g_vulkan = NULL;

const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;
bool framebufferResized = false;

static VkBool32 check_validation_layers(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers) {
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

static void instance_layers(struct vulkan *vulkan, VkResult err) {
    uint32_t instance_layer_count = 0;
    char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
    vulkan->enabled_layer_count = 0;

    VkBool32 validation_found = 0;
    if (vulkan->validate) {
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        assert(!err);

        if (instance_layer_count > 0) {
            VkLayerProperties *instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
            assert(!err);

            validation_found = check_validation_layers(ARRAY_SIZE(validation_layers), validation_layers, instance_layer_count, instance_layers);
        
            if (validation_found) {
                vulkan->enabled_layer_count = ARRAY_SIZE(validation_layers);
                vulkan->enabled_layers[0] = "VK_LAYER_KHRONOS_validation";
            }
            free(instance_layers);
        }

        if (!validation_found) {
            fprintf(stderr, "Failed to find required validation layers.\n\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void instance_extensions(struct vulkan *vulkan) {
    // Get required extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    if (glfwExtensionCount == 0) {
        fprintf(stderr, "GLFW failed to provide required Vulkan extensions\n");
        exit(EXIT_FAILURE);
    }

    // Copy GLFW extensions - cast away const to match your struct type
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        vulkan->extension_names[vulkan->enabled_extension_count++] = (char*)glfwExtensions[i];
        assert(vulkan->enabled_extension_count < 64);
    }

    // Add debug utils extension if validation is enabled
    if (vulkan->validate) {
        vulkan->extension_names[vulkan->enabled_extension_count++] = (char*)VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        assert(vulkan->enabled_extension_count < 64);
    }
}

static void create_vulkan_instance(struct vulkan *vulkan) {
    VkResult err;

    instance_layers(vulkan, err);
    instance_extensions(vulkan);

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VKRenderer for desktop engine",
        .pNext = NULL,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .pNext = NULL,
        .enabledLayerCount = vulkan->enabled_layer_count,
        .ppEnabledLayerNames = (const char *const *)vulkan->enabled_layers,
        .enabledExtensionCount = vulkan->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)vulkan->extension_names,
    };

    err = vkCreateInstance(&createInfo, NULL, &vulkan->instance);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        fprintf(stderr, "Cannot find a compatible Vulkan installable client driver (ICD)\n");
        exit(EXIT_FAILURE);
    } else if (err == VK_ERROR_LAYER_NOT_PRESENT) {
        fprintf(stderr, "Cannot find a specified layer\n");
        exit(EXIT_FAILURE);
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        fprintf(stderr, "Cannot find a specified extension library\n");
        exit(EXIT_FAILURE);
    } else if (err) {
        fprintf(stderr, "vkCreateInstanceFailed\n");
        exit(EXIT_FAILURE);
    }
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = {0};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.isGraphicsFamilySet = true;
            break;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
            indices.isPresentFamilySet = true;
        }
    }


    return indices;
}

static bool check_device_extensions_support(struct vulkan *vulkan, VkPhysicalDevice physical_device) {
    VkResult err;

    uint32_t device_extensions_count = 0;
    VkBool32 swapchainExtFound = 0;
    vulkan->enabled_extension_count = 0;
    memset(vulkan->extension_names, 0, sizeof(vulkan->extension_names));
    
    err = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &device_extensions_count, NULL);
    assert(!err);

    if (device_extensions_count > 0) {
        VkExtensionProperties *device_extensions = malloc(sizeof(VkExtensionProperties) * device_extensions_count);
        err = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &device_extensions_count, device_extensions);
        assert(!err);

        for (uint32_t i = 0; i < device_extensions_count; i++) {
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName)) {
                swapchainExtFound = 1;
                vulkan->extension_names[vulkan->enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            };
        }

        assert(vulkan->enabled_extension_count < 64);
        free(device_extensions);
    }

    return swapchainExtFound;
}

static uint32_t rate_gpu_suitability(struct vulkan *vulkan, VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score = 0;

    // We prefer discrete gpus
    // We prefer maximum possible size of textures
    // Pick only supporting geometry shadows
    // Check gpu support queue families and prefer gpu that have both graphics and present
    // Check gpu support device extensions

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    QueueFamilyIndices indices = find_queue_families(device, surface);
    if (!indices.isGraphicsFamilySet) {
        printf("Queue Family not supported!\n");
        return 0;
    }

    if (!check_device_extensions_support(vulkan, device)) {
        fprintf(stderr, "GPU doesn't support required extensions\n");
        return 0;
    }

    return score;
}

static void pick_gpu(struct vulkan *vulkan) {
    VkResult err;

    uint32_t deviceCount = 0;
    err = vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, NULL);
    assert(!err);

    if (deviceCount == 0) {
        fprintf(stderr, "failed to find GPUs with Vulkan support!");
        exit(EXIT_FAILURE);
    }

    VkPhysicalDevice devices[deviceCount];
    err = vkEnumeratePhysicalDevices(vulkan->instance, &deviceCount, devices);
    assert(!err);

    VkPhysicalDevice device;
    uint32_t deviceScore = 0;
    for (uint32_t i = 0; i < deviceCount; i++) {
        uint32_t score = rate_gpu_suitability(vulkan, devices[i], vulkan->surface);
        if (score > deviceScore) {
            deviceScore = score;
            device = devices[i];
        }
    }

    if (device == NULL) {
        fprintf(stderr, "Failed to find a stuitable GPU!\n");
        exit(EXIT_FAILURE);
    }
    vulkan->gpu = device;
 
    // Log info
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    printf(
        "\nSelected device %s\n\n",
        properties.deviceName
    );

    vulkan->queue_family_indices = find_queue_families(device, vulkan->surface);
}

static void get_family_device_queues(VkDeviceQueueCreateInfo *queues, QueueFamilyIndices indices) {
  // GraphicsQueue
  VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.graphicsFamily,
    .queueCount = 1
  };

  float graphicsQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
  queues[0] = graphicsQueueCreateInfo;

  // PresentQueue
  VkDeviceQueueCreateInfo presentQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.presentFamily,
    .queueCount = 1
  };

  float presentQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
  queues[1] = presentQueueCreateInfo;
}

static void create_logical_device(struct vulkan *vulkan) {
    VkResult err;
    QueueFamilyIndices indices = find_queue_families(vulkan->gpu, vulkan->surface);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(vulkan->gpu, &deviceFeatures);

    VkDeviceQueueCreateInfo queues[2];
    get_family_device_queues(queues, indices);

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        //.pQueueCreateInfos = &queueCreateInfo,
        .pQueueCreateInfos = queues,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = vulkan->enabled_extension_count, 
        .ppEnabledExtensionNames = (const char *const *)vulkan->extension_names, // (const char *const *)vulkan->extension_names,
        .enabledLayerCount = 0
    };

    if (vulkan->validate) {
        createInfo.enabledLayerCount = vulkan->enabled_layer_count;
        createInfo.ppEnabledLayerNames = (const char *const *)vulkan->enabled_layers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    err = vkCreateDevice(vulkan->gpu, &createInfo, NULL, &vulkan->device);
    assert(!err);

    vkGetDeviceQueue(vulkan->device, vulkan->queue_family_indices.graphicsFamily, 0, &vulkan->graphics_queue);
    vkGetDeviceQueue(vulkan->device, vulkan->queue_family_indices.presentFamily, 0, &vulkan->present_queue);
}

SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    details.formatCount = formatCount;
    VkSurfaceFormatKHR formats[formatCount];
    details.formats = formats;
    if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, NULL);
    details.presentModeCount = presentCount;
    VkPresentModeKHR presentModes[presentCount];
    details.presentModes = presentModes;
    if (presentCount != 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, details.presentModes);
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(const VkSurfaceFormatKHR *surface_formats, uint32_t count) {
    // Prefer non-SRGB formats
    for (uint32_t i = 0; i < count; i++) {
        const VkFormat format = surface_formats[i].format;

         if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
            format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
            format == VK_FORMAT_A1R5G5B5_UNORM_PACK16 || format == VK_FORMAT_R5G6B5_UNORM_PACK16 ||
            format == VK_FORMAT_R16G16B16A16_SFLOAT) {
            return surface_formats[i];
        }
    }

    printf("Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");

    // assert(count >= 1);
    return surface_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(uint32_t modes_count, VkPresentModeKHR *available_modes) {
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

        actualExtent.width = clamp_uint32_t(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = clamp_uint32_t(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static void create_swapchain(struct vulkan *vulkan) {
    VkResult err;
    SwapChainSupportDetails swapchain_support = query_swapchain_support(vulkan->gpu, vulkan->surface);

    VkSurfaceFormatKHR surf_format = choose_swap_surface_format(swapchain_support.formats, swapchain_support.formatCount);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.presentModeCount, swapchain_support.presentModes);
    VkExtent2D extent = choose_swap_extent(g_window, swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount+1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vulkan->surface,
        .minImageCount = image_count,
        .imageFormat = surf_format.format,
        .imageColorSpace = surf_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    QueueFamilyIndices indices = find_queue_families(vulkan->gpu, vulkan->surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }

    createInfo.preTransform = swapchain_support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    err = vkCreateSwapchainKHR(vulkan->device, &createInfo, NULL, &vulkan->swapchain);
    assert(!err);

    vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain, &image_count, NULL);
    vulkan->swapchain_images = (VkImage*)malloc(sizeof(VkImage) * image_count);

    vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain, &image_count, vulkan->swapchain_images);
    vulkan->swapchain_image_count = image_count;

    vulkan->swapchain_image_format = surf_format.format;
    vulkan->swapchain_extent = extent;
}

static void create_image_views(struct vulkan *vulkan) {
    VkResult err;
    vulkan->swapchain_image_views =  (VkImageView*)malloc(sizeof(VkImageView) * vulkan->swapchain_image_count);

    for (uint32_t i = 0; i < vulkan->swapchain_image_count; i++) {
        VkImageViewCreateInfo cI = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vulkan->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vulkan->swapchain_image_format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        };

        err = vkCreateImageView(vulkan->device, &cI, NULL, &vulkan->swapchain_image_views[i]);
        assert(!err);
    }
}

static void create_render_pass(struct vulkan *vulkan) {
    VkResult err;
    
    VkAttachmentDescription colorAttachment = {
        .format = vulkan->swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    err = vkCreateRenderPass(vulkan->device, &renderPassInfo, NULL, &vulkan->render_pass);
    assert(!err);
}

static void create_graphics_pipeline(struct vulkan *vulkan) {
    ShaderFile vert = {0};
    ShaderFile frag = {0};

    readFile("build/shaders/vertex.vert.spv", &vert);
    readFile("build/shaders/fragment.frag.spv", &frag);

    VkShaderModule vertModule = create_shader_module(vulkan, &vert);
    VkShaderModule fragModule = create_shader_module(vulkan, &frag);

     VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input - нам не нужны вершины, используем полноэкранный треугольник
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    
    // Viewport и scissor
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)vulkan->swapchain_extent.width,
        .height = (float)vulkan->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = vulkan->swapchain_extent
    };
    
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE, // Не отсекать - полноэкранный треугольник
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE
    };
    
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };
    
    // Descriptor set layout для uniform буферов и текстур
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    
    VkDescriptorSetLayoutBinding bindings[] = {samplerLayoutBinding, uboLayoutBinding};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = bindings
    };
    
    if (vkCreateDescriptorSetLayout(vulkan->device, &layoutInfo, NULL, &vulkan->descriptor_set_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        exit(EXIT_FAILURE);
    }
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vulkan->descriptor_set_layout
    };
    
    if (vkCreatePipelineLayout(vulkan->device, &pipelineLayoutInfo, NULL, &vulkan->pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        exit(EXIT_FAILURE);
    }
    
    // Создаем pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .layout = vulkan->pipeline_layout,
        .renderPass = vulkan->render_pass,
        .subpass = 0
    };
    
    if (vkCreateGraphicsPipelines(vulkan->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vulkan->graphics_pipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        exit(EXIT_FAILURE);
    }
    
    // Очистка
    vkDestroyShaderModule(vulkan->device, vertModule, NULL);
    vkDestroyShaderModule(vulkan->device, fragModule, NULL);
    free(vert.code);    // Освобождаем память из ShaderFile vert
    free(frag.code);    // Освобождаем память из ShaderFile frag
}

static void create_framebuffers(struct vulkan *vulkan) {
    vulkan->swapchain_framebuffers = (VkFramebuffer*)malloc(vulkan->swapchain_image_count * sizeof(VkFramebuffer));

    for (uint32_t i = 0; i < vulkan->swapchain_image_count; i++) {
        VkImageView attachments[] = { vulkan->swapchain_image_views[i] };

        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkan->render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vulkan->swapchain_extent.width;
        framebufferInfo.height = vulkan->swapchain_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vulkan->device, &framebufferInfo, NULL, &vulkan->swapchain_framebuffers[i]) != VK_SUCCESS) {
            printf("failed to create framebuffer!\n");
            exit(10);
        }
    }
}

static void create_command_pool(struct vulkan *vulkan) {
    QueueFamilyIndices indices = find_queue_families(vulkan->gpu, vulkan->surface);
    
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphicsFamily
    };
    
    if (vkCreateCommandPool(vulkan->device, &poolInfo, NULL, &vulkan->command_pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        exit(EXIT_FAILURE);
    }
}

static void create_command_buffers(struct vulkan *vulkan) {
    vulkan->command_buffers = malloc(sizeof(VkCommandBuffer) * vulkan->swapchain_image_count);
    
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkan->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)vulkan->swapchain_image_count
    };
    
    if (vkAllocateCommandBuffers(vulkan->device, &allocInfo, vulkan->command_buffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        exit(EXIT_FAILURE);
    }
}

static void create_sync_objects(struct vulkan *vulkan) {
    vulkan->image_available_semaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    vulkan->render_finished_semaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    vulkan->in_flight_fences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vulkan->device, &semaphoreInfo, NULL, &vulkan->image_available_semaphores[i]) != VK_SUCCESS) {
            printf("Failed to create imageAvailableSemaphore!\n");
            exit(15);
        }
        if (vkCreateSemaphore(vulkan->device, &semaphoreInfo, NULL, &vulkan->render_finished_semaphores[i]) != VK_SUCCESS) {
            printf("Failed to create renderFinishedSemaphore!\n");
            exit(15);
        }
        if (vkCreateFence(vulkan->device, &fenceInfo, NULL, &vulkan->in_flight_fences[i]) != VK_SUCCESS) {
            printf("Failed to create fence!\n");
            exit(15);
        }
    }
}

void init_vulkan() {
    g_vulkan = calloc(1, sizeof(struct vulkan));
    g_vulkan->validate = true;

    create_vulkan_instance(g_vulkan);
    create_surface(g_vulkan->instance, &g_vulkan->surface);
    pick_gpu(g_vulkan);
    create_logical_device(g_vulkan);
    create_swapchain(g_vulkan);
    create_image_views(g_vulkan);
    create_render_pass(g_vulkan);
    create_graphics_pipeline(g_vulkan);
    create_framebuffers(g_vulkan);
    create_command_pool(g_vulkan);
    create_command_buffers(g_vulkan);
    create_sync_objects(g_vulkan);

    printf("Vulkan initialized!\nCan start rendering.\n");
}

static void cleanup_swapchain(struct vulkan *vulkan) {
  for (uint32_t i = 0; i < vulkan->swapchain_image_count; i++) {
    vkDestroyFramebuffer(vulkan->device, vulkan->swapchain_framebuffers[i], NULL);
  }

  for (uint32_t i = 0; i < vulkan->swapchain_image_count; i++) {
    vkDestroyImageView(vulkan->device, vulkan->swapchain_image_views[i], NULL);
  }

  vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain, NULL);
}

void cleanup_vulkan() {
    printf("Cleanup started");

    cleanup_swapchain(g_vulkan);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(g_vulkan->device, g_vulkan->image_available_semaphores[i], NULL);
        vkDestroySemaphore(g_vulkan->device, g_vulkan->render_finished_semaphores[i], NULL);
        vkDestroyFence(g_vulkan->device, g_vulkan->in_flight_fences[i], NULL);
    }

    vkDestroyCommandPool(g_vulkan->device, g_vulkan->command_pool, NULL);

    vkDestroyPipeline(g_vulkan->device, g_vulkan->graphics_pipeline, NULL);
    vkDestroyPipelineLayout(g_vulkan->device, g_vulkan->pipeline_layout, NULL);
    vkDestroyRenderPass(g_vulkan->device, g_vulkan->render_pass, NULL);

    // if (vulkan->validate) {
    //     DestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debug_messenger, NULL);
    // }
    vkDestroyDevice(g_vulkan->device, NULL);
    vkDestroySurfaceKHR(g_vulkan->instance, g_vulkan->surface, NULL);
    vkDestroyInstance(g_vulkan->instance, NULL);

    free(g_vulkan);
    g_vulkan = NULL;
}

void device_idle() {
    vkDeviceWaitIdle(g_vulkan->device);
}

VkShaderModule create_shader_module(struct vulkan *vulkan, ShaderFile *shaderFile) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderFile->size,
        .pCode = (uint32_t*)shaderFile->code
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vulkan->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module!\n");
        exit(7);
    }

    return shaderModule;
}

void readFile(const char *filename, ShaderFile *shader) {
    FILE *pFile;

    pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        printf("Failed to open %s\n", filename);
        exit(7);
    }

    fseek(pFile, 0L, SEEK_END);
    shader->size = ftell(pFile);

    fseek(pFile, 0L, SEEK_SET);

    shader->code = (char*)malloc(sizeof(char) * shader->size);
    size_t readCount = fread(shader->code, shader->size, sizeof(char), pFile);
    //   printf("ReadCount: %ld\n", readCount);

    fclose(pFile);
}

static void record_command_buffer(struct vulkan *vulkan, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("failed to begin recording command buffer!\n");
    exit(13);
  }

  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vulkan->render_pass;
  renderPassInfo.framebuffer = vulkan->swapchain_framebuffers[imageIndex];
  renderPassInfo.renderArea.offset.x = 0;
  renderPassInfo.renderArea.offset.y = 0;
  renderPassInfo.renderArea.extent = vulkan->swapchain_extent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->graphics_pipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)vulkan->swapchain_extent.width;
  viewport.height = (float)vulkan->swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor= {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = vulkan->swapchain_extent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    printf("failed to record command buffer!\n");
    exit(14);
  }
}

static void recreate_swapchain(struct vulkan *vulkan) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(g_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(g_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkan->device);

    cleanup_swapchain(vulkan);

    create_swapchain(vulkan);
    create_image_views(vulkan);
    create_framebuffers(vulkan);
}

void draw_frame(struct vulkan *vulkan) {
    vkWaitForFences(vulkan->device, 1, &vulkan->in_flight_fences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(vulkan->device, 1, &vulkan->in_flight_fences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vulkan->device, vulkan->swapchain, UINT64_MAX, vulkan->image_available_semaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(vulkan);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("Failed to acquire swap chain image!\n");
        exit(17);
    }

    // Only reset the fence if we are submitting work
    vkResetFences(vulkan->device, 1, &vulkan->in_flight_fences[currentFrame]);

    vkResetCommandBuffer(vulkan->command_buffers[currentFrame], 0);
    record_command_buffer(vulkan, vulkan->command_buffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { vulkan->image_available_semaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan->command_buffers[currentFrame];

    VkSemaphore signalSemaphores[] = { vulkan->render_finished_semaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkan->graphics_queue, 1, &submitInfo, vulkan->in_flight_fences[currentFrame]) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer!\n");
        exit(16);
    }

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { vulkan->swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = NULL; // Optional

    VkResult queueResult = vkQueuePresentKHR(vulkan->present_queue, &presentInfo);

    if (queueResult == VK_ERROR_OUT_OF_DATE_KHR || queueResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreate_swapchain(vulkan);
    } else if (queueResult != VK_SUCCESS) {
        printf("Failed to present swap chain image!\n");
        exit(17);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}