#include <logger.h>
#include <config.h>
#include <wayland/server.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <dbus-ipc/dbus-server.h>
#include <dbus-ipc/dbus-wayland-integration.h>
#include <dbus-ipc/dbus-core.h>

#include "dbus-ipc/modules/test-module/test-module.h"

#define EXIT_AND_ERROR(msg) \
    do { \
        LOG_ERROR(LOG_MODULE_CORE, msg); \
        wl_display_destroy(server.display); \
        logger_cleanup();\
    } while(0)

static struct server *global_server = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

static void signal_handler(int signal) {
    (void)signal; // Shut up compiler
    g_shutdown_requested = 1;
    g_logger_graceful_shutdown = 1;
    
    // const char* msg = "terminated\n";
    // write(STDOUT_FILENO, msg, strlen(msg));
    
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
    struct dbus_wayland_integration_data *dbus_wayland_integration_data = dbus_wayland_integration_create(server.display);
    struct dbus_server *dbus_server = dbus_server_create();

    if (!dbus_server_init(dbus_server, dbus_wayland_integration_data)) {
        dbus_wayland_integration_cleanup(dbus_wayland_integration_data);
        dbus_core_cleanup(dbus_server);
        EXIT_AND_ERROR("Failed to init dbus server");
    }

    /* Register modules here */
    if (!test_module_register(dbus_server)) {
        LOG_WARN(LOG_MODULE_CORE, "Failed to register test module");
    } else {
        LOG_INFO(LOG_MODULE_CORE, "Test module registered successfully");
        
        // Пример отправки тестового события через 2 секунды
        struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
        
        test_module_send_event(dbus_server, "startup", "Server started successfully");
        
        /* Run module self-test */
        if (test_module_run_full_test(dbus_server)) {
            LOG_INFO(LOG_MODULE_CORE, "dbus test-module self-test passed");
        } else {
            LOG_ERROR(LOG_MODULE_CORE, "dbus test-module self-test failed");
        }
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

    LOG_INFO(LOG_MODULE_CORE, "DesktopEngine server shutdown complete");

    /* Unregister modules here */
    test_module_unregister(dbus_server);

    dbus_wayland_integration_cleanup(dbus_wayland_integration_data);
    dbus_core_cleanup(dbus_server);
    server_cleanup(&server);
    logger_cleanup();

    return 0;
}