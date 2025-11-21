#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "xdg-shell-protocol.h"
#include "logger/logger.h"
#include <wayland-server.h>

/* TODO
Create shm pool methods (To log client pixel buffers)
*/

static void bind_xdg_wm_base(struct wl_client *client, void *data,
                            uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &xdg_wm_base_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    xdg_wm_base_send_ping(resource, 1234); // Пример ping
    SERVER_DEBUG("XDG shell bound to client");
}

static void shm_create_pool(struct wl_client *client,
                           struct wl_resource *shm_resource,
                           uint32_t id, int fd, int32_t size) {
    SERVER_DEBUG("Creating SHM pool: fd=%d, size=%d", fd, size);
}

static const struct wl_shm_interface shm_implementation = {
    .create_pool = shm_create_pool,
};

static void bind_shm(struct wl_client *client, void *data,
                            uint32_t version, uint32_t id) {
    struct server *server = data;
    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_shm_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &shm_implementation, data, NULL);
    
    // shm_send_formats(client, resource);
    
    SERVER_DEBUG("SHM bound to client");
}

static void compositor_create_surface(struct wl_client *client,
                                     struct wl_resource *resource,
                                     uint32_t id) {
    struct server *server = wl_resource_get_user_data(resource);
    
    struct wl_resource *surface_resource = wl_resource_create(
        client, &wl_surface_interface, wl_resource_get_version(resource), id);
    
    if (!surface_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    // Создаем структуру поверхности
    struct surface *surface = calloc(1, sizeof(struct surface));
    if (!surface) {
        wl_client_post_no_memory(client);
        wl_resource_destroy(surface_resource);
        return;
    }
    
    surface->resource = surface_resource;
    wl_list_init(&surface->link);
    
    // Добавляем поверхность в список сервера
    wl_list_insert(&server->surfaces, &surface->link);
    
    wl_resource_set_implementation(surface_resource, NULL, surface, NULL);
    
    SERVER_DEBUG("Surface created");
};

static void compositor_create_region(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t id) {
    struct wl_resource *region_resource = wl_resource_create(
        client, &wl_region_interface, 1, id);
    
    if (!region_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    SERVER_DEBUG("Region created");
};

static const struct wl_compositor_interface compositor_implementation = {
    .create_surface = compositor_create_surface,
    .create_region = compositor_create_region,
};

static void bind_compositor(struct wl_client *client, void *data,
                           uint32_t version, uint32_t id) {    
    struct wl_resource *resource = wl_resource_create(
        client, &wl_compositor_interface, version, id);
    
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    
    wl_resource_set_implementation(resource, &compositor_implementation, data, NULL);
    
    SERVER_DEBUG("Compositor bound to client");
}

static void handle_client_destroyed(struct wl_listener *listener, void *data) {
    struct client *client = wl_container_of(listener, client, destroy_listener);
    wl_list_remove(&client->link);
    free(client);

    SERVER_INFO("Client destroyed and removed from server list");
}

static void handle_client_created(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, client_created_listener);
    struct wl_client *wl_client = data;

    struct client *client = calloc(1, sizeof(struct client));
    if (!client) {
        SERVER_ERROR("Failed to allocate client");
        return;
    }
    client->wl_client = wl_client;
    wl_list_init(&client->link);
    wl_list_insert(&server->clients, &client->link);  // в начало

    client->destroy_listener.notify = handle_client_destroyed;
    wl_client_add_destroy_listener(wl_client, &client->destroy_listener);

    SERVER_INFO("New client connected (total clients: %d)", 
               wl_list_length(&server->clients));
}

void server_init(struct server *server) {
    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Init clients list */
    wl_list_init(&server->clients);

    /* Create wayland globals */
    server->xdg_wm_base_global = wl_global_create(
        server->display,
        &xdg_wm_base_interface,
        1, server, bind_xdg_wm_base
    );

    server->shm_global = wl_global_create(
        server->display, 
        &wl_shm_interface, 
        1, server, bind_shm
    );

    server->compositor_global = wl_global_create(
        server->display, 
        &wl_compositor_interface, 
        4, server, bind_compositor
    );

    if (!server->xdg_wm_base_global || !server->shm_global || !server->compositor_global) {
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
    /* TODO
        Адекватное закрытие клиентов, с их terminate
    */
    // struct surface *surface, *tmp_surface;
    // wl_list_for_each_safe(surface, tmp_surface, &server->surfaces, link) {
    //     wl_list_remove(&surface->link);
    //     free(surface);
    // }
    
    // // Cleanup clients
    // struct client *client, *tmp_client;
    // wl_list_for_each_safe(client, tmp_client, &server->clients, link) {
    //     wl_list_remove(&client->link);
    //     free(client);
    // }

    if (server->display) {
        wl_display_destroy(server->display);
        server->display = NULL;
    }
}