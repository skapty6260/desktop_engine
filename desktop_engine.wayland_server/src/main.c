/* TODO
Basic wayland server (Open clients, create surfaces)

-- In logger_fatal not just cleaning up, but work with shutdown requests

Buffer methods
-- Shm buffer creation handling
-- Attach buffer (No surface implementation)

Ipc bridge (Should send clients info, should receive input events)
-- Simple socket server to send buffers (No receiving & broadcast yet)
*/
#include <logger/logger.h>
#include <config/config.h>
#include <server/server.h>
#include <ipc/network_server.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

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

static void *network_server_thread(void *arg) {
    struct server *server = (struct server *)arg;
    
    if (network_server_init(PORT) != 0) {
        LOG_ERROR(LOG_MODULE_CORE, "Failed to initialize network server");
        return NULL;
    }
    
    LOG_INFO(LOG_MODULE_CORE, "Network server started on port %d", PORT);
    
    // Здесь можно добавить логику для получения событий от клиентов
    // или просто оставить сервер работающим для отправки буферов
    
    while (!g_shutdown_requested) {
        sleep(1); // Просто ждем, пока основной поток отправляет буферы
    }
    
    network_server_cleanup();
    LOG_INFO(LOG_MODULE_CORE, "Network server stopped");
    
    return NULL;
}

int main(int argc, char **argv) {
    logger_config_t logger_config;
    server_config_t server_config;
    struct server server = {0};
    pthread_t network_thread;
    
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
        // TODO (cleanup)
        wl_display_destroy(server.display);
        SERVER_FATAL("Failed to add socket for Wayland display");
    }
    setenv("WAYLAND_DISPLAY", server.socket, true);

    /* Запускаем сетевой сервер в отдельном потоке */
    if (pthread_create(&network_thread, NULL, network_server_thread, &server) != 0) {
        LOG_ERROR(LOG_MODULE_CORE, "Failed to create network server thread");
        server_cleanup(&server);
        logger_cleanup();
        return 1;
    }

    /* Test bed (test client) */
    if (server_config.startup_cmd) {
        LOG_DEBUG(LOG_MODULE_CORE, "TESTBED startup cmd: %s", server_config.startup_cmd);
        if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", server_config.startup_cmd, (void *)NULL);
		}
    }

    LOG_INFO(LOG_MODULE_CORE, "Wayland server started on socket: %s", server.socket);
    LOG_INFO(LOG_MODULE_CORE, "Network server started on port: %d", PORT);
    LOG_INFO(LOG_MODULE_CORE, "Press Ctrl+C to stop the server");

    while (g_shutdown_requested != 1) {
        struct buffer *buf = calloc(1, sizeof(struct buffer));
        buf->width = 1920;
        buf->height = 1080;
        buf->resource = NULL;
        buf->size = 400000;
        buf->type = WL_BUFFER_SHM;

        network_server_broadcast_buffer(buf);
    }

    server_run(&server);

    LOG_INFO(LOG_MODULE_CORE, "Wayland server shutdown complete");
    
    // Ждем завершения сетевого потока
    pthread_join(network_thread, NULL);

    server_cleanup(&server);
    logger_cleanup();

    return 0;
}