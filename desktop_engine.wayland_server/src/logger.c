#include <logger.h>
#include <errno.h>
#include <signal.h>

// Глобальные переменные
static logger_config_t g_config;
static FILE* g_log_file = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_logger_initialized = 0;

// Очередь сообщений
static log_message_t g_message_queue[1000];
static int g_queue_head = 0;
static int g_queue_tail = 0;
static int g_queue_size = 0;
static pthread_t g_worker_thread;
static volatile sig_atomic_t g_worker_running = 0;

// Глобальный флаг graceful shutdown
volatile sig_atomic_t g_logger_graceful_shutdown = 0;

// Получение строкового представления уровня
static const char* level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// Получение строкового представления модуля
static const char* module_to_string(log_module_t module) {
    switch (module) {
        case LOG_MODULE_SERVER:    return "SERVER";
        case LOG_MODULE_DBUS:  return "DBUS";
        case LOG_MODULE_CORE:        return "CORE";
        default: return "UNKNOWN";
    }
}

// Получение цвета для уровня
static log_color_t level_to_color(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return LOG_COLOR_CYAN;
        case LOG_LEVEL_INFO:  return LOG_COLOR_GREEN;
        case LOG_LEVEL_WARN:  return LOG_COLOR_YELLOW;
        case LOG_LEVEL_ERROR: return LOG_COLOR_RED;
        case LOG_LEVEL_FATAL: return LOG_COLOR_BRIGHT_RED;
        default: return LOG_COLOR_WHITE;
    }
}

// Установка цвета терминала
static void set_terminal_color(log_color_t color) {
    if (g_config.use_colors && g_config.log_to_console) {
        printf("\033[1;%dm", color);
    }
}

// Сброс цвета терминала
static void reset_terminal_color() {
    if (g_config.use_colors && g_config.log_to_console) {
        printf("\033[0m");
    }
}

// Форматирование времени
static void format_timestamp(char* buffer, size_t size, time_t timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Добавление сообщения в очередь
static int enqueue_message(const log_message_t* message) {
    if (g_logger_graceful_shutdown) {
        return 0; // Не принимаем новые сообщения во время shutdown
    }
    
    pthread_mutex_lock(&g_log_mutex);
    
    if (g_queue_size >= 1000) {
        pthread_mutex_unlock(&g_log_mutex);
        return 0;
    }
    
    g_message_queue[g_queue_tail] = *message;
    g_queue_tail = (g_queue_tail + 1) % 1000;
    g_queue_size++;
    
    pthread_mutex_unlock(&g_log_mutex);
    return 1;
}

// Извлечение сообщения из очереди
static int dequeue_message(log_message_t* message) {
    pthread_mutex_lock(&g_log_mutex);
    
    if (g_queue_size == 0) {
        pthread_mutex_unlock(&g_log_mutex);
        return 0; // Очередь пуста
    }
    
    *message = g_message_queue[g_queue_head];
    g_queue_head = (g_queue_head + 1) % 1000;
    g_queue_size--;
    
    pthread_mutex_unlock(&g_log_mutex);
    return 1;
}

// Запись сообщения в файл
static void write_to_file(const log_message_t* message) {
    if (!g_config.log_to_file || !g_log_file) return;
    
    char timestamp[32];
    format_timestamp(timestamp, sizeof(timestamp), message->timestamp);
    
    fprintf(g_log_file, "[%s] [%s] [%s] %s\n",
            timestamp,
            level_to_string(message->level),
            module_to_string(message->module),
            message->message);
    
    if (g_config.flush_immediately) {
        fflush(g_log_file);
    }
}

// Вывод сообщения в консоль
static void write_to_console(const log_message_t* message) {
    if (!g_config.log_to_console) return;
    
    char timestamp[32];
    format_timestamp(timestamp, sizeof(timestamp), message->timestamp);
    
    set_terminal_color(level_to_color(message->level));
    
    printf("[%s] [%s] [%s] %s",
           timestamp,
           level_to_string(message->level),
           module_to_string(message->module),
           message->message);
    
    reset_terminal_color();
    printf("\n");
    
    fflush(stdout);
}

// Обработка сообщения
static void process_message(const log_message_t* message) {
    write_to_console(message);
    write_to_file(message);
    
    // Обработка FATAL уровня
    if (message->level == LOG_LEVEL_FATAL) {
        fprintf(stderr, "FATAL error occurred. Terminating application.\n");
        logger_cleanup();
        exit(EXIT_FAILURE);
    }
}

// Функция рабочего потока
static void* worker_thread(void* arg) {
    (void)arg;
    
    while (g_worker_running || g_queue_size > 0) {
        log_message_t message;
        
        if (dequeue_message(&message)) {
            process_message(&message);
        } else {
            // Если очередь пуста и идет shutdown - выходим
            if (g_logger_graceful_shutdown) {
                break;
            }
            sleep(0);
        }
    }
    
    return NULL;
}

// Инициализация логгера
int logger_init(const logger_config_t* config) {
    if (g_logger_initialized) {
        return 1;
    }
    
    g_config = *config;
    
    // Открытие файла лога
    if (g_config.log_to_file && g_config.log_file_path[0] != '\0') {
        g_log_file = fopen(g_config.log_file_path, "a");
        if (!g_log_file) {
            fprintf(stderr, "ERROR: Cannot open log file: %s\n", g_config.log_file_path);
            g_config.log_to_file = 0;
        }
    }
    
    // Запуск асинхронного рабочего потока
    if (g_config.async_enabled) {
        g_worker_running = 1;
        if (pthread_create(&g_worker_thread, NULL, worker_thread, NULL) != 0) {
            fprintf(stderr, "ERROR: Cannot create logger worker thread\n");
            g_config.async_enabled = 0;
        }
    }
    
    g_logger_initialized = 1;
    
    LOG_INFO(LOG_MODULE_CORE, "Logger initialized. Level: %s, Async: %s", 
             level_to_string(g_config.level),
             g_config.async_enabled ? "enabled" : "disabled");
    
    return 1;
}

// Освобождение ресурсов (упрощенная версия)
void logger_cleanup(void) {
    if (!g_logger_initialized) return;
    
    // Устанавливаем флаг graceful shutdown
    g_logger_graceful_shutdown = 1;
    
    // Остановка рабочего потока
    if (g_config.async_enabled && g_worker_running) {
        g_worker_running = 0;
        
        // Простая версия - ждем завершения потока без таймаута
        pthread_join(g_worker_thread, NULL);
    }
    
    // Закрытие файла
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    g_logger_initialized = 0;
    g_logger_graceful_shutdown = 0; // Сбрасываем флаг
}

// Основная функция логирования
void logger_log(log_level_t level, log_module_t module, const char* file, int line, 
                const char* function, const char* format, ...) {
    if (!g_logger_initialized || level < g_config.level) {
        return;
    }
    
    log_message_t message;
    message.level = level;
    message.module = module;
    message.file = file;
    message.line = line;
    message.function = function;
    message.timestamp = time(NULL);
    
    // Форматирование сообщения
    va_list args;
    va_start(args, format);
    vsnprintf(message.message, sizeof(message.message), format, args);
    va_end(args);
    
    // Синхронная или асинхронная обработка
    if (g_config.async_enabled) {
        if (!enqueue_message(&message)) {
            // Если очередь переполнена, пишем синхронно
            process_message(&message);
        }
    } else {
        process_message(&message);
    }
}