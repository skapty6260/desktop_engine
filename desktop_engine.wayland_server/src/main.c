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
    
    // Run the server in blocking mode
    network_server_run();  // This will accept connections
    
    // This code won't execute until server stops
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

    // Should be removed
    sleep(1);  // Give time for server to start and client to connect

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

    // Test buffer cross-network ipc
    struct buffer *test_buffer = calloc(1, sizeof(struct buffer));
    if (!test_buffer) {
        LOG_ERROR(LOG_MODULE_CORE, "Failed to allocate test buffer");
    } else {
        test_buffer->format = PIXEL_FORMAT_ARGB8888;
        test_buffer->height = 300;  // Match your debug output: 400x300
        test_buffer->width = 400;
        test_buffer->type = WL_BUFFER_SHM;
        test_buffer->size = 400 * 300 * 4;  // ARGB8888 = 4 bytes per pixel
        test_buffer->shm.stride = 400 * 4;  // width * bytes per pixel
        
        // Allocate and fill with test pattern
        test_buffer->shm.data = malloc(test_buffer->size);
        if (test_buffer->shm.data) {
            // Fill with a simple pattern
            uint32_t *pixels = (uint32_t*)test_buffer->shm.data;
            for (int i = 0; i < 400 * 300; i++) {
                pixels[i] = 0xFF0000FF;  // Red color
            }
            
            // Send the buffer
            network_server_broadcast_buffer(test_buffer);
            
            free(test_buffer->shm.data);
        }
        free(test_buffer);
    }

    server_run(&server);

    LOG_INFO(LOG_MODULE_CORE, "Wayland server shutdown complete");
    
    // Ждем завершения сетевого потока
    pthread_join(network_thread, NULL);

    server_cleanup(&server);
    logger_cleanup();

    return 0;
}