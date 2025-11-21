#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

typedef enum {
    LOG_MODULE_SERVER,
    LOG_MODULE_IPC_GET,
    LOG_MODULE_IPC_SEND,
    LOG_MODULE_CORE
} log_module_t;

typedef enum {
    LOG_COLOR_BLACK = 30,
    LOG_COLOR_RED = 31,
    LOG_COLOR_GREEN = 32,
    LOG_COLOR_YELLOW = 33,
    LOG_COLOR_BLUE = 34,
    LOG_COLOR_MAGENTA = 35,
    LOG_COLOR_CYAN = 36,
    LOG_COLOR_WHITE = 37,
    LOG_COLOR_BRIGHT_RED = 91
} log_color_t;

typedef struct {
    log_level_t level;
    log_module_t module;
    const char* file;
    int line;
    const char* function;
    time_t timestamp;
    char message[1024];
} log_message_t;

typedef struct {
    log_level_t level;
    int use_colors;
    int log_to_file;
    int log_to_console;
    int async_enabled;
    int flush_immediately;
    char log_file_path[256];
} logger_config_t;

// Добавляем флаг graceful shutdown
extern volatile sig_atomic_t g_logger_graceful_shutdown;

int logger_init(const logger_config_t* config);
void logger_cleanup(void);
void logger_log(log_level_t level, log_module_t module, const char* file, int line, 
                const char* function, const char* format, ...);

// Log macros
#define LOG_DEBUG(module, ...) logger_log(LOG_LEVEL_DEBUG, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(module, ...)  logger_log(LOG_LEVEL_INFO, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(module, ...)  logger_log(LOG_LEVEL_WARN, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(module, ...) logger_log(LOG_LEVEL_ERROR, module, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(module, ...) logger_log(LOG_LEVEL_FATAL, module, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Server log macros
#define SERVER_DEBUG(...)    LOG_DEBUG(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_INFO(...)     LOG_INFO(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_WARN(...)     LOG_WARN(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_ERROR(...)    LOG_ERROR(LOG_MODULE_SERVER, __VA_ARGS__)
#define SERVER_FATAL(...)    LOG_FATAL(LOG_MODULE_SERVER, __VA_ARGS__)

#define IPC_GET_DEBUG(...)       LOG_DEBUG(LOG_MODULE_IPC_GET, __VA_ARGS__)
#define IPC_GET_INFO(...)        LOG_INFO(LOG_MODULE_IPC_GET, __VA_ARGS__)
#define IPC_GET_WARN(...)        LOG_WARN(LOG_MODULE_IPC_GET, __VA_ARGS__)
#define IPC_GET_ERROR(...)       LOG_ERROR(LOG_MODULE_IPC_GET, __VA_ARGS__)
#define IPC_GET_FATAL(...)       LOG_FATAL(LOG_MODULE_IPC_GET, __VA_ARGS__)

#define IPC_SEND_DEBUG(...)       LOG_DEBUG(LOG_MODULE_IPC_SEND, __VA_ARGS__)
#define IPC_SEND_INFO(...)        LOG_INFO(LOG_MODULE_IPC_SEND, __VA_ARGS__)
#define IPC_SEND_WARN(...)        LOG_WARN(LOG_MODULE_IPC_SEND, __VA_ARGS__)
#define IPC_SEND_ERROR(...)       LOG_ERROR(LOG_MODULE_IPC_SEND, __VA_ARGS__)
#define IPC_SEND_FATAL(...)       LOG_FATAL(LOG_MODULE_IPC_SEND, __VA_ARGS__)

#endif