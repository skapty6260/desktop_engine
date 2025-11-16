/* TODO
Basic wayland server (Open clients, create surfaces)
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include "logger/logger.h"
#include "logger/logger_config.h"
#include "server.h"
#include <unistd.h>

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
    
    LOG_INFO(LOG_MODULE_CORE, "Starting windows server");

    // Wayland server and ipc
    struct server *server = calloc(1, sizeof(struct server));
    if (!server) {
        LOG_FATAL(LOG_MODULE_CORE, "Failed to allocate memory for wayland server");
    }

    server_init(server);
    
    LOG_FATAL(LOG_MODULE_CORE, "Testing fatal");

    LOG_INFO(LOG_MODULE_CORE, "Exiting windows server");
    
    logger_cleanup();

    return 0;
}