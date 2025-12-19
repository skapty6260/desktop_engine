#include <logger.h>
#include <config.h>
#include <wayland/server.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <dbus-server/server.h>
#include <dbus-server/modules/buffer_module.h>

#define EXIT_AND_ERROR(msg) \
    do { \
        LOG_ERROR(LOG_MODULE_CORE, msg); \
        wl_display_destroy(server.display); \
        logger_cleanup();\
    } while(0)

static struct server *global_server = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

static void signal_handler(int signal) {
    (void)signal;
    g_shutdown_requested = 1;
    g_logger_graceful_shutdown = 1;
    
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
        LOG_FATAL(LOG_MODULE_CORE, "XDG_RUNTIME_DIR is not set in the environment. Aborting.");
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
        EXIT_AND_ERROR("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server.socket, true);

    /* Run D-Bus server */
    struct dbus_server *dbus_server = dbus_create_server("org.skapty6260.DesktopEngine");
    if (!dbus_server) {
        EXIT_AND_ERROR("Failed to create dbus server");
    }

    /* Создание и добавление тестового модуля */
    DBUS_MODULE *test_module = create_test_module();
    if (test_module) {
        dbus_server_add_module(dbus_server, test_module);
        LOG_INFO(LOG_MODULE_CORE, "Test module registered to D-Bus server");
    } else {
        LOG_WARN(LOG_MODULE_CORE, "Failed to create test module");
    }

    if (dbus_start_main_loop(dbus_server) != 0) {
        if (dbus_server->bus_name) {
            release_bus_name(dbus_server->connection, dbus_server->bus_name);
            free(dbus_server->bus_name);
            dbus_server->bus_name = NULL;    
        }
        if (dbus_server->connection) {
            dbus_connection_unref(dbus_server->connection);
            dbus_server->connection = NULL;
        }
        free(dbus_server);
        EXIT_AND_ERROR("Failed to run dbus main loop in different thread");
    }

    /* Test bed (test client) */
    if (server_config.startup_cmd) {
        LOG_DEBUG(LOG_MODULE_CORE, "TESTBED startup cmd: %s", server_config.startup_cmd);
        if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", server_config.startup_cmd, (void *)NULL);
		}
    }

    LOG_INFO(LOG_MODULE_CORE, "DesktopEngine server started on socket: %s", server.socket);

    server_run(&server);

    LOG_INFO(LOG_MODULE_CORE, "DesktopEngine server shutdown");

    dbus_server_cleanup(dbus_server);
    server_cleanup(&server);
    logger_cleanup();

    return 0;
}