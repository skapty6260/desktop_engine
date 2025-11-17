/* TODO
Basic wayland server (Open clients, create surfaces)

-- In logger_fatal not just cleaning up, but work with shutdown requests

Ipc bridge (Should send clients info, should receive input events)
*/
#define _POSIX_C_SOURCE 200112L
#include "logger/logger.h"
#include "config/config.h"
#include "server.h"
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

static struct server *global_server = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

static void signal_handler(int signal) {
    g_shutdown_requested = 1;
    g_logger_graceful_shutdown = 1;
    
    const char* msg = "terminated\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    if (global_server && global_server->display) {
        wl_display_terminate(global_server->display);
    }
}

int main(int argc, char **argv) {
    logger_config_t logger_config;
    server_config_t server_config;
    struct server server = {0};
    
    /* Logger and config initialization */
    load_default_config(&logger_config, &server_config);
    parse_args(argc, argv, &logger_config, &server_config);
    if (!logger_init(&logger_config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    if (!getenv("XDG_RUNTIME_DIR")) {
        SERVER_FATAL("XDG_RUNTIME_DIR is not set in the environment. Aborting.");
    }

    server_init(&server);

    /* Signal handling for graceful shutdown */
    global_server = &server;
    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0  // SA_RESTART
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    server.socket = wl_display_add_socket_auto(server.display);
    if (!server.socket) {
        wl_display_destroy(server.display);
        SERVER_FATAL("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server.socket, true);

    /* Test bed (test client) */
    if (server_config.startup_cmd) {
        LOG_DEBUG(LOG_MODULE_CORE, "TESTBED startup cmd: %s", server_config.startup_cmd);
        if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", server_config.startup_cmd, (void *)NULL);
		}
    }

    LOG_INFO(LOG_MODULE_CORE, "Wayland server started on socket: %s", server.socket);
    LOG_INFO(LOG_MODULE_CORE, "Press Ctrl+C to stop the server");

    server_run(&server);

    LOG_INFO(LOG_MODULE_CORE, "Wayland server shutdown complete");
    
    server_cleanup(&server);
    logger_cleanup();

    return 0;
}