#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

typedef enum {
    WL_BUFFER_SHM = 0,      // Shared memory buffer
    WL_BUFFER_DMA_BUF = 1,  // DMA-BUF buffer
    WL_BUFFER_EGL = 2       // EGL buffer
} WlBufferType;

typedef struct BufferData {
    uint32_t width;         // Ширина буфера в пикселях
    uint32_t height;        // Высота буфера в пикселях
    uint32_t stride;        // Stride (байт на строку)
    uint32_t format;        // Формат пикселей (например, 0x1 для WL_SHM_FORMAT_ARGB8888)
    const char *format_str; // Строковое представление формата
    size_t size;            // Размер данных в байтах
    bool has_data;          // Есть ли данные
    WlBufferType type;      // Тип буфера: WL_BUFFER_SHM, WL_BUFFER_DMA_BUF, WL_BUFFER_EGL
    
    // Поля для данных (будут заполнены при обработке)
    void *data;             // Указатель на данные буфера
    uint64_t timestamp;     // Временная метка получения
    uint32_t frame_number;  // Номер кадра
} BufferData;

typedef struct BufferUpdate {
    bool has_update;        // Есть ли новое обновление
    BufferData data;        // Данные буфера
    const char* info;       // Дополнительная информация из DBus сообщения
} BufferUpdate;

bool buffer_fetcher_init(void);
void buffer_fetcher_cleanup(void);
bool buffer_fetcher_get_update(BufferUpdate *update);
void buffer_fetcher_reset_update(void);
BufferData buffer_fetcher_get_current_buffer(void);
bool buffer_fetcher_wait_update(uint32_t timeout_ms);