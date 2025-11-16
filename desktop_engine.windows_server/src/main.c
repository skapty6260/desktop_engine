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
    struct server server = {0};
    
    /* Logger initialization */
    // Default logger config
    logger_config_default(&config);
    // Parse args and pass to logger config
    logger_config_parse_args(argc, argv, &config);
    if (!logger_init(&config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    /* Wayland server initialization */
    // struct server *server = calloc(1, sizeof(struct server));
    // if (!server) {
    //     LOG_FATAL(LOG_MODULE_CORE, "Failed to allocate memory for wayland server");
    // }

    if (!getenv("XDG_RUNTIME_DIR")) {
        SERVER_FATAL("XDG_RUNTIME_DIR is not set in the environment. Aborting.")
    }

    server_init(server);
    server_run(server);
    
    LOG_FATAL(LOG_MODULE_CORE, "Testing fatal");

    LOG_INFO(LOG_MODULE_CORE, "Exiting windows server");
    
    logger_cleanup();

    return 0;
}