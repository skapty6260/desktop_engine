#define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "logger/logger.h"
#include <wayland-server.h>

/* TODO
Полностью переделать систему обработки клиентов.
Найти способ получать графический вывод клиентов для передачи на ipc calls.
*/

static void bind_compositor(
    struct wl_client *client, void *data,
    uint32_t version, uint32_t id
) {
    struct server *server = data;
    struct wl_resource *resource = wl_resource_create(
        client, 
        &wl_compositor_interface, 
        version, id
    );
    wl_resource_set_implementation(resource, &compositor_interface, server, NULL);
    SERVER_DEBUG("Client bound compositor (version: %u, id: %u)", version, id);
}

static void bind_shell(
    struct wl_client *client, void *data,
    uint32_t version, uint32_t id
) {
    struct server *server = data;
    struct wl_resource *resource = wl_resource_create(
        client, 
        &wl_shell_interface, 
        version, id
    );
    wl_resource_set_implementation(resource, &shell_interface, server, NULL);
    SERVER_DEBUG("Client bound shell (version: %u, id: %u)", version, id);
}

static void bind_shm(
    struct wl_client *client, void *data,
    uint32_t version, uint32_t id
) {
    struct server *server = data;
    struct wl_resource *resource = wl_resource_create(
        client, 
        &wl_shm_interface, 
        version, id
    );
    wl_resource_set_implementation(resource, &shm_interface, server, NULL);
    SERVER_DEBUG("Client bound SHM (version: %u, id: %u)", version, id);
}

static void handle_client_created(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, client_created_listener);
    struct wl_client *wl_client = data;

    struct client *client = malloc(sizeof(*client));
    if (!client) {
        SERVER_ERROR("Failed to allocate client structure");
        return;
    }

    client->wl_client = wl_client;
    client->pid = wl_client_get_credentials(wl_client, NULL, NULL, NULL);

    wl_list_insert(&server->clients, &client->link);

    SERVER_INFO("New client connected (PID: %d)", client->pid);

    struct wl_listener *destroy_listener = malloc(sizeof(*destroy_listener));
    destroy_listener->notify = handle_client_destroyed;
    wl_client_add_destroy_listener(wl_client, destroy_listener);
}

static void handle_client_destroyed(struct wl_listener *listener, void *data) {
    struct wl_client *wl_client = data;
    
    // Находим клиента в списке
    struct server *server = NULL; // Нужно получить server из контекста
    struct client *client, *tmp;
    wl_list_for_each_safe(client, tmp, &server->clients, link) {
        if (client->wl_client == wl_client) {
            SERVER_INFO("Client disconnected (PID: %d)", client->pid);
            wl_list_remove(&client->link);
            free(client);
            break;
        }
    }
    
    free(listener);
}

void server_init(struct server *server) {
    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Init clients list */
    wl_list_init(&server->clients);

    /* Create wayland globals */
    server->compositor_global = wl_global_create(
        server->display,
        &wl_compositor_interface,
        4, server, bind_compositor
    );

    server->shell_global = wl_global_create(
        server->display,
        &wl_shell_interface,
        1, server, bind_compositor
    );

    server->shm_global = wl_global_create(
        server->display, 
        &wl_shm_interface, 
        1, server, bind_shm
    );

    if (!server->compositor_global || !server->shell_global || !server->shm_global) {
        SERVER_FATAL("Failed to create Wayland globals");
    }

    /* Assign event listeners */
    server->client_created_listener.notify = handle_client_created;
    wl_display_add_client_created_listener(server->display, &server->client_created_listener);
}

void server_run(struct server *server) {
    SERVER_INFO("Wayland server is running");
    wl_display_run(server->display);
}

void server_cleanup(struct server *server) {
    struct client *client, *tmp;
    wl_list_for_each_safe(client, tmp, &server->clients, link) {
        wl_list_remove(&client->link);
        free(client);
    }

    if (server->display) {
        wl_display_destroy(server->display);
        server->display = NULL;
    }
}