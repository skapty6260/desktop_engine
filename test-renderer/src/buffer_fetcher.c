#include <buffer_fetcher.h>
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

static DBusConnection *g_conn = NULL;
static pthread_mutex_t g_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static BufferUpdate g_current_update = {0};
static bool g_initialized = false;
static bool g_has_new_update = false;

static bool parse_buffer_info(const char *info_str, BufferData *buffer) {
    if (!info_str || !buffer) return false;
    
    // Пример строки: "Buffer updated: 400x300, stride=1600, format=0x1, size=480000, type=SHM"
    
    // Парсим ширину и высоту
    if (sscanf(info_str, "Buffer updated: %ux%u", &buffer->width, &buffer->height) != 2) {
        // Попробуем другой формат
        if (sscanf(info_str, "Width: %u, Height: %u", &buffer->width, &buffer->height) != 2) {
            return false;
        }
    }
    
    // Парсим stride
    char *stride_ptr = strstr(info_str, "stride=");
    if (stride_ptr) {
        sscanf(stride_ptr, "stride=%u", &buffer->stride);
    }
    
    // Парсим format
    char *format_ptr = strstr(info_str, "format=");
    if (format_ptr) {
        if (strstr(format_ptr, "0x1") || strstr(info_str, "ARGB8888")) {
            buffer->format = 0x1;
            buffer->format_str = "WL_SHM_FORMAT_ARGB8888";
        } else if (strstr(format_ptr, "0x34325241") || strstr(info_str, "ARGB2101010")) {
            buffer->format = 0x34325241;
            buffer->format_str = "WL_SHM_FORMAT_ARGB2101010";
        } else if (strstr(format_ptr, "0x34325258") || strstr(info_str, "XRGB2101010")) {
            buffer->format = 0x34325258;
            buffer->format_str = "WL_SHM_FORMAT_XRGB2101010";
        } else {
            // Пробуем распарсить как hex
            unsigned int format_hex;
            if (sscanf(format_ptr, "format=0x%x", &format_hex) == 1) {
                buffer->format = format_hex;
                buffer->format_str = "Unknown format";
            }
        }
    }
    
    // Парсим size
    char *size_ptr = strstr(info_str, "size=");
    if (size_ptr) {
        sscanf(size_ptr, "size=%zu", &buffer->size);
    }
    
    // Парсим type
    char *type_ptr = strstr(info_str, "type=");
    if (type_ptr) {
        if (strstr(type_ptr, "SHM")) {
            buffer->type = WL_BUFFER_SHM;
        } else if (strstr(type_ptr, "DMA_BUF")) {
            buffer->type = WL_BUFFER_DMA_BUF;
        } else if (strstr(type_ptr, "EGL")) {
            buffer->type = WL_BUFFER_EGL;
        }
    }
    
    buffer->has_data = (buffer->width > 0 && buffer->height > 0);
    
    // Генерируем временную метку и номер кадра
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    buffer->timestamp = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    static uint32_t frame_counter = 0;
    buffer->frame_number = frame_counter++;
    
    return buffer->has_data;
}

// Обработчик DBus сообщений
static DBusHandlerResult message_handler(DBusConnection *connection,
                                         DBusMessage *message,
                                         void *user_data) {
    (void)connection;
    (void)user_data;
    
    if (dbus_message_is_signal(message, 
        "org.skapty6260.DesktopEngine.Buffer", 
        "Updated")) {
        
        DBusError err;
        dbus_error_init(&err);
        
        char *info = NULL;
        if (dbus_message_get_args(message, &err, 
                                 DBUS_TYPE_STRING, &info,
                                 DBUS_TYPE_INVALID)) {
            
            pthread_mutex_lock(&g_buffer_mutex);
            
            // Парсим информацию о буфере
            if (parse_buffer_info(info, &g_current_update.data)) {
                g_current_update.has_update = true;
                g_current_update.info = strdup(info); // Копируем строку
                g_has_new_update = true;
                
                printf("Buffer update received: %ux%u, stride=%u, format=0x%x, size=%zu, type=%d\n",
                       g_current_update.data.width,
                       g_current_update.data.height,
                       g_current_update.data.stride,
                       g_current_update.data.format,
                       g_current_update.data.size,
                       g_current_update.data.type);
            } else {
                printf("Failed to parse buffer info: %s\n", info);
            }
            
            pthread_mutex_unlock(&g_buffer_mutex);
        } else {
            fprintf(stderr, "Failed to parse DBus signal: %s\n", err.message);
            dbus_error_free(&err);
        }
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

// Инициализация DBus соединения
bool buffer_fetcher_init(void) {
    if (g_initialized) return true;
    
    DBusError err;
    dbus_error_init(&err);
    
    // Подключаемся к сессионной шине
    g_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus connection error: %s\n", err.message);
        dbus_error_free(&err);
        return false;
    }
    
    // Запрашиваем имя
    int ret = dbus_bus_request_name(g_conn, 
                                    "org.skapty6260.DesktopEngine.BufferClient",
                                    DBUS_NAME_FLAG_REPLACE_EXISTING,
                                    &err);
    
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus name error: %s\n", err.message);
        dbus_connection_unref(g_conn);
        g_conn = NULL;
        dbus_error_free(&err);
        return false;
    }
    
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        fprintf(stderr, "Could not acquire DBus name: %d\n", ret);
        dbus_connection_unref(g_conn);
        g_conn = NULL;
        return false;
    }
    
    // Добавляем фильтр для сигналов
    dbus_bus_add_match(g_conn, 
        "type='signal',"
        "interface='org.skapty6260.DesktopEngine.Buffer',"
        "path='/org/skapty6260/DesktopEngine/Buffer',"
        "sender='org.skapty6260.DesktopEngine'",
        &err);
    
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus match error: %s\n", err.message);
        dbus_connection_unref(g_conn);
        g_conn = NULL;
        dbus_error_free(&err);
        return false;
    }
    
    // Добавляем фильтр с функцией-обработчиком
    if (!dbus_connection_add_filter(g_conn, message_handler, NULL, NULL)) {
        fprintf(stderr, "Failed to add DBus filter\n");
        dbus_connection_unref(g_conn);
        g_conn = NULL;
        return false;
    }
    
    g_initialized = true;
    printf("Buffer fetcher initialized successfully\n");
    return true;
}

// Деинициализация
void buffer_fetcher_cleanup(void) {
    if (!g_initialized) return;
    
    pthread_mutex_lock(&g_buffer_mutex);
    
    if (g_current_update.info) {
        free((void*)g_current_update.info);
        g_current_update.info = NULL;
    }
    
    if (g_current_update.data.data) {
        free(g_current_update.data.data);
        g_current_update.data.data = NULL;
    }
    
    memset(&g_current_update, 0, sizeof(g_current_update));
    g_has_new_update = false;
    
    pthread_mutex_unlock(&g_buffer_mutex);
    
    if (g_conn) {
        dbus_connection_remove_filter(g_conn, message_handler, NULL);
        dbus_connection_unref(g_conn);
        g_conn = NULL;
    }
    
    pthread_mutex_destroy(&g_buffer_mutex);
    g_initialized = false;
    printf("Buffer fetcher cleaned up\n");
}

// Получение обновлений (неблокирующее)
bool buffer_fetcher_get_update(BufferUpdate *update) {
    if (!g_initialized || !update) return false;
    
    pthread_mutex_lock(&g_buffer_mutex);
    
    // Обрабатываем все ожидающие сообщения
    while (dbus_connection_read_write_dispatch(g_conn, 0)) {
        // Читаем все сообщения в очереди
    }
    
    if (g_has_new_update) {
        // Копируем обновление
        *update = g_current_update;
        g_has_new_update = false;
        
        pthread_mutex_unlock(&g_buffer_mutex);
        return true;
    }
    
    pthread_mutex_unlock(&g_buffer_mutex);
    return false;
}

// Сброс состояния обновления
void buffer_fetcher_reset_update(void) {
    pthread_mutex_lock(&g_buffer_mutex);
    g_has_new_update = false;
    pthread_mutex_unlock(&g_buffer_mutex);
}

// Получение текущего состояния буфера
BufferData buffer_fetcher_get_current_buffer(void) {
    BufferData buffer = {0};
    
    pthread_mutex_lock(&g_buffer_mutex);
    buffer = g_current_update.data;
    pthread_mutex_unlock(&g_buffer_mutex);
    
    return buffer;
}

// Ожидание нового обновления (блокирующее)
bool buffer_fetcher_wait_update(uint32_t timeout_ms) {
    if (!g_initialized) return false;
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    uint64_t timeout_ns = (uint64_t)timeout_ms * 1000000ULL;
    
    while (true) {
        pthread_mutex_lock(&g_buffer_mutex);
        
        // Обрабатываем DBus сообщения
        dbus_connection_read_write_dispatch(g_conn, 0);
        
        if (g_has_new_update) {
            pthread_mutex_unlock(&g_buffer_mutex);
            return true;
        }
        
        pthread_mutex_unlock(&g_buffer_mutex);
        
        // Проверяем таймаут
        if (timeout_ms > 0) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t elapsed_ns = (uint64_t)(now.tv_sec - start.tv_sec) * 1000000000ULL +
                                 (now.tv_nsec - start.tv_nsec);
            
            if (elapsed_ns >= timeout_ns) {
                return false;
            }
        }
        
        sleep(1000); // 1ms
    }
}