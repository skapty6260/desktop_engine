/* TODO
Basic wayland server (Open clients, create surfaces)
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include "logger/logger.h"
#include "config/config.h"
#include "server.h"
#include <unistd.h>

int main(int argc, char **argv) {
    logger_config_t logger_config;
    server_config_t server_config;
    struct server server = {0};
    
    /* Logger initialization */
    logger_config_default(&logger_config);
    server_config_default(&server_config);
    parse_args(argc, argv, &logger_config, &server_config);
    if (!logger_init(&logger_config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    /* Wayland server initialization */
    // struct server *server = calloc(1, sizeof(struct server));
    // if (!server) {
    //     LOG_FATAL(LOG_MODULE_CORE, "Failed to allocate memory for wayland server");
    // }

    if (!getenv("XDG_RUNTIME_DIR")) {
        SERVER_FATAL("XDG_RUNTIME_DIR is not set in the environment. Aborting.");
    }

    printf("Startup command %s\n", server_config.startup_cmd);
    server_init(&server);
    server_run(&server);
    
    LOG_FATAL(LOG_MODULE_CORE, "Testing fatal");

    LOG_INFO(LOG_MODULE_CORE, "Exiting windows server");
    
    server_cleanup();
    logger_cleanup();

    return 0;
}