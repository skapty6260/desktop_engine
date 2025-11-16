/* TODO
Basic wayland server (Open clients, create surfaces)
Signal handler for good exiting and cleanup
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include "logger/logger.h"
#include "config/config.h"
#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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

    printf("Startup cmd: %s\n", server_config.startup_cmd);
    server_init(&server);

    /* Add UNIX socket and try use startup cmd. */
    server.socket = wl_display_add_socket_auto(server.display);
    if (!server.socket) {
        wl_display_destroy(server.display);
        SERVER_FATAL("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server.socket, true);

    if (server_config.startup_cmd) {
        if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", server_config.startup_cmd, (void *)NULL);
		}
    }

    server_run(&server);

    LOG_INFO(LOG_MODULE_CORE, "Exiting windows server");
    
    server_cleanup(&server);
    logger_cleanup();

    return 0;
}