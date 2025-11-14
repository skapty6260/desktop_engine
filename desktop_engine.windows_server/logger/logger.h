#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>

// Уровни логирования
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

// Модули приложения
typedef enum {
    LOG_MODULE_SERVER,
    LOG_MODULE_IPC_GET,
    LOG_MODULE_IPC_SEND,
} log_module_t;

// Цвета для терминала
typedef enum {
    LOG_COLOR_RESET = 0,
    LOG_COLOR_RED = 31,
    LOG_COLOR_GREEN = 32,
    LOG_COLOR_YELLOW = 33,
    LOG_COLOR_BLUE = 34,
    LOG_COLOR_MAGENTA = 35,
    LOG_COLOR_CYAN = 36,
    LOG_COLOR_WHITE = 37,
    LOG_COLOR_BRIGHT_RED = 91,
    LOG_COLOR_BRIGHT_GREEN = 92
} log_color_t;

// Структура сообщения лога
typedef struct {
    log_level_t level;
    log_module_t module;
    const char* file;
    int line;
    const char* function;
    char message[1024];
    time_t timestamp;
} log_message_t;

// Структура конфигурации
typedef struct {
    log_level_t level;
    int use_colors;
    int log_to_file;
    int log_to_console;
    char log_file_path[256];
    int async_enabled;
    int flush_immediately;
} logger_config_t;

// Инициализация логгера
int logger_init(const logger_config_t* config);
// Освобождение ресурсов
void logger_cleanup(void);
// Основная функция логирования
void logger_log(log_level_t level, log_module_t module, const char* file, int line, const char* function, const char* format, ...);

// Макросы для удобного использования
#define LOG_DEBUG(module, ...)   logger_log(LOG_LEVEL_DEBUG, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(module, ...)    logger_log(LOG_LEVEL_INFO, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(module, ...)    logger_log(LOG_LEVEL_WARN, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(module, ...)   logger_log(LOG_LEVEL_ERROR, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(module, ...)   logger_log(LOG_LEVEL_FATAL, module, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Макросы для конкретных модулей
#define SERVER_DEBUG(...)    LOG_DEBUG(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_INFO(...)     LOG_INFO(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_WARN(...)     LOG_WARN(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_ERROR(...)    LOG_ERROR(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_FATAL(...)    LOG_FATAL(LOG_MODULE_SERVER, __VA_ARGS__)

#define IPC_DEBUG(...)       LOG_DEBUG(LOG_MODULE_IPC, __VA_ARGS__)
#define IPC_INFO(...)        LOG_INFO(LOG_MODULE_IPC, __VA_ARGS__)
#define IPC_WARN(...)        LOG_WARN(LOG_MODULE_IPC, __VA_ARGS__)
#define IPC_ERROR(...)       LOG_ERROR(LOG_MODULE_IPC, __VA_ARGS__)
#define IPC_FATAL(...)       LOG_FATAL(LOG_MODULE_IPC, __VA_ARGS__)

#endif