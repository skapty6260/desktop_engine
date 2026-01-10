#include <vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint32_t find_memory_type(struct vulkan *vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan->gpu, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    fprintf(stderr, "Failed to find suitable memory type!\n");
    exit(EXIT_FAILURE);
}

// Создание staging буфера для копирования данных
void create_staging_buffer(struct vulkan *vulkan, size_t size) {
    VkResult err;
    
    // Если staging буфер уже существует и достаточного размера, используем его
    if (vulkan->staging_buffer && vulkan->staging_buffer_size >= size) {
        return;
    }
    
    // Освобождаем старый staging буфер если есть
    if (vulkan->staging_buffer) {
        vkDestroyBuffer(vulkan->device, vulkan->staging_buffer, NULL);
        vkFreeMemory(vulkan->device, vulkan->staging_buffer_memory, NULL);
    }
    
    vulkan->staging_buffer_size = size;
    
    // Создаем staging буфер
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    
    err = vkCreateBuffer(vulkan->device, &bufferInfo, NULL, &vulkan->staging_buffer);
    assert(!err);
    
    // Выделяем память
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan->device, vulkan->staging_buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = find_memory_type(vulkan, memRequirements.memoryTypeBits, 
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };
    
    err = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &vulkan->staging_buffer_memory);
    assert(!err);
    
    vkBindBufferMemory(vulkan->device, vulkan->staging_buffer, vulkan->staging_buffer_memory, 0);
    
    printf("Staging buffer created: %zu bytes\n", size);
}

// Создание текстуры
void create_texture_image(struct vulkan *vulkan, uint32_t width, uint32_t height) {
    VkResult err;
    
    // Освобождаем старую текстуру если есть
    if (vulkan->texture_image) {
        vkDestroyImage(vulkan->device, vulkan->texture_image, NULL);
        vkFreeMemory(vulkan->device, vulkan->texture_image_memory, NULL);
        vkDestroyImageView(vulkan->device, vulkan->texture_image_view, NULL);
    }
    
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM, // Соответствует XRGB8888
        .extent = { width, height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    
    err = vkCreateImage(vulkan->device, &imageInfo, NULL, &vulkan->texture_image);
    assert(!err);
    
    // Выделяем память
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkan->device, vulkan->texture_image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = find_memory_type(vulkan, memRequirements.memoryTypeBits, 
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    
    err = vkAllocateMemory(vulkan->device, &allocInfo, NULL, &vulkan->texture_image_memory);
    assert(!err);
    
    vkBindImageMemory(vulkan->device, vulkan->texture_image, vulkan->texture_image_memory, 0);
    
    // Создаем image view
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vulkan->texture_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    
    err = vkCreateImageView(vulkan->device, &viewInfo, NULL, &vulkan->texture_image_view);
    assert(!err);
    
    printf("Texture image created: %ux%u\n", width, height);
}

// Создание сэмплера
void create_texture_sampler(struct vulkan *vulkan) {
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };
    
    VkResult err = vkCreateSampler(vulkan->device, &samplerInfo, NULL, &vulkan->texture_sampler);
    assert(!err);
}

// Создание дескрипторного пула
void create_descriptor_pool(struct vulkan *vulkan) {
    VkDescriptorPoolSize poolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes,
        .maxSets = 1,
    };
    
    VkResult err = vkCreateDescriptorPool(vulkan->device, &poolInfo, NULL, &vulkan->descriptor_pool);
    assert(!err);
}

// Создание дескрипторного набора
void create_descriptor_set(struct vulkan *vulkan) {
    VkDescriptorSetLayout layouts[] = { vulkan->descriptor_set_layout };
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vulkan->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = layouts,
    };
    
    VkResult err = vkAllocateDescriptorSets(vulkan->device, &allocInfo, &vulkan->descriptor_set);
    assert(!err);
    
    // Обновляем дескрипторы
    VkDescriptorImageInfo imageInfo = {
        .sampler = vulkan->texture_sampler,
        .imageView = vulkan->texture_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    
    VkWriteDescriptorSet descriptorWrites[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vulkan->descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        }
    };
    
    vkUpdateDescriptorSets(vulkan->device, 1, descriptorWrites, 0, NULL);
}

// Обновление текстуры Vulkan из SHM буфера
void update_vulkan_texture_from_buffer(struct vulkan *vulkan, RenderBuffer_t *buffer) {
    if (!vulkan || !buffer || !buffer->data || buffer->size == 0) {
        printf("Invalid buffer for texture update\n");
        return;
    }
    
    printf("Updating Vulkan texture from buffer: %ux%u, %zu bytes\n", 
           buffer->width, buffer->height, buffer->size);
    
    VkResult err;
    
    // 1. Создаем staging буфер если нужно
    create_staging_buffer(vulkan, buffer->size);
    
    // 2. Копируем данные из SHM в staging буфер
    void *staging_data;
    err = vkMapMemory(vulkan->device, vulkan->staging_buffer_memory, 0, buffer->size, 0, &staging_data);
    assert(!err);
    
    memcpy(staging_data, buffer->data, buffer->size);
    
    VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = vulkan->staging_buffer_memory,
        .offset = 0,
        .size = buffer->size,
    };
    vkFlushMappedMemoryRanges(vulkan->device, 1, &range);
    
    vkUnmapMemory(vulkan->device, vulkan->staging_buffer_memory);
    
    // 3. Создаем текстуру если нужно (или меняем размер)
    if (!vulkan->texture_image || 
        vulkan->current_buffer == NULL ||
        vulkan->current_buffer->width != buffer->width ||
        vulkan->current_buffer->height != buffer->height) {
        
        create_texture_image(vulkan, buffer->width, buffer->height);
        
        // Обновляем дескрипторы с новой текстурой
        VkDescriptorImageInfo imageInfo = {
            .sampler = vulkan->texture_sampler,
            .imageView = vulkan->texture_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vulkan->descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };
        
        vkUpdateDescriptorSets(vulkan->device, 1, &descriptorWrite, 0, NULL);
    }
    
    // 4. Создаем командный буфер для копирования
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkan->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkan->device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    // 5. Переводим текстуру в layout для записи
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = vulkan->texture_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
    
    // 6. Копируем из staging буфера в текстуру
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = buffer->stride / 4, // 4 байта на пиксель для XRGB8888
        .bufferImageHeight = buffer->height,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {buffer->width, buffer->height, 1},
    };
    
    vkCmdCopyBufferToImage(
        commandBuffer,
        vulkan->staging_buffer,
        vulkan->texture_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    
    // 7. Переводим текстуру в layout для чтения шейдером
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
    
    vkEndCommandBuffer(commandBuffer);
    
    // 8. Отправляем командный буфер
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    
    vkQueueSubmit(vulkan->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan->graphics_queue);
    
    vkFreeCommandBuffers(vulkan->device, vulkan->command_pool, 1, &commandBuffer);
    
    // 9. Сохраняем ссылку на текущий буфер
    vulkan->current_buffer = buffer;
    vulkan->texture_needs_update = false;
    
    printf("Vulkan texture updated successfully\n");
}