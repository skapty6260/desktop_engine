/* TODO
Basic wayland server (Open clients, create surfaces)
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include "logger/logger.h"
#include "logger/logger_config.h"
#include <unistd.h>

// Пример функций сервера
void server_function(void) {
    SERVER_DEBUG("Server starting initialization");
    SERVER_INFO("Server listening on port 8080");
    SERVER_WARN("High memory usage detected");
    SERVER_ERROR("Failed to accept client connection");
    // SERVER_FATAL("Critical server failure"); // Раскомментировать для теста FATAL
}

// Пример функций IPC
void ipc_function(void) {
    IPC_GET_DEBUG("IPC module initializing");
    IPC_SEND_INFO("IPC message queue created");
    IPC_GET_WARN("IPC buffer 80%% full");
    IPC_SEND_ERROR("IPC connection timeout");
}

int main(int argc, char **argv) {
    logger_config_t config;
    
    // Конфигурация по умолчанию
    logger_config_default(&config);
    
    // Парсинг аргументов командной строки
    logger_config_parse_args(argc, argv, &config);
    
    // Инициализация логгера
    if (!logger_init(&config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    LOG_INFO(LOG_MODULE_CORE, "Application started");
    
    // Демонстрация работы модулей
    server_function();
    ipc_function();
    
    // Демонстрация разных уровней логирования
    LOG_DEBUG(LOG_MODULE_CORE, "Debug message with value: %d", 42);
    LOG_INFO(LOG_MODULE_CORE, "Info message with string: %s", "test");
    LOG_WARN(LOG_MODULE_CORE, "Warning message");
    LOG_ERROR(LOG_MODULE_CORE, "Error message");
    
    // Имитация работы
    for (int i = 0; i < 5; i++) {
        LOG_INFO(LOG_MODULE_SERVER, "Processing request %d", i);
        sleep(100);
    }
    
    LOG_INFO(LOG_MODULE_CORE, "Application finished");
    
    // Очистка ресурсов
    logger_cleanup();

    return 0;
}