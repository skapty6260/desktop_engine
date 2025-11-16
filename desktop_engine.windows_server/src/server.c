#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE
#include <sys/signalfd.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>

static struct server *global_server = NULL;
static int signal_fd[2] = {-1, -1};

static void signal_handler(int signal) {
    // Просто записываем в pipe
    int saved_errno = errno;
    write(signal_fd[1], &signal, sizeof(signal));
    errno = saved_errno;
}

static int handle_signal_event(int fd, uint32_t mask, void *data) {
    int signal;
    ssize_t result = read(fd, &signal, sizeof(signal));
    
    if (result != sizeof(signal)) {
        return 1;
    }
    
    LOG_INFO(LOG_MODULE_CORE, "Received signal %d, initiating graceful shutdown...", signal);
    
    if (global_server && global_server->display) {
        wl_display_terminate(global_server->display);
    }
    
    return 1;
}

static void setup_signal_handlers(void) {
    // Создаем pipe для коммуникации
    if (pipe(signal_fd) == -1) {
        perror("pipe");
        exit(1);
    }
    
    // Делаем pipe неблокирующим
    int flags = fcntl(signal_fd[0], F_GETFL, 0);
    fcntl(signal_fd[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(signal_fd[1], F_GETFL, 0);
    fcntl(signal_fd[1], F_SETFL, flags | O_NONBLOCK);
    
    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART
    };
    
    sigemptyset(&sa.sa_mask);
    
    // Handle SIGINT and SIGTERM
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    // Ignore SIGPIPE
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}

void server_init(struct server *server) {
    SERVER_DEBUG("Initializing wayland server...");

    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Signal handling */
    global_server = server;
    setup_signal_handlers();
    if (signal_fd[0] != -1) {
        struct wl_event_loop *event_loop = wl_display_get_event_loop(server->display);
        wl_event_loop_add_fd(event_loop, signal_fd[0], WL_EVENT_READABLE, handle_signal_event, NULL);
    }

    SERVER_DEBUG("Wayland server initialized");
}

void server_run(struct server *server) {
    SERVER_INFO("Wayland server is running");
    wl_display_run(server->display);
    SERVER_INFO("Wayland server event loop stopped");
}

void server_cleanup(struct server *server) {
    /* Close signal pipe */
    if (signal_fd[0] != -1) {
        close(signal_fd[0]);
        signal_fd[0] = -1;
    }
    if (signal_fd[1] != -1) {
        close(signal_fd[1]);
        signal_fd[1] = -1;
    }

    if (server->display) {
        wl_display_destroy(server->display);
        server->display = NULL;
    }
}