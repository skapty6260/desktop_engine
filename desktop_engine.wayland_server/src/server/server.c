#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include <server/server.h>
#include <logger/logger.h>
#include <wayland-server.h>

#include <server/wayland/compositor.h>
#include <server/wayland/shm.h>
#include <server/xdg-shell/xdg.h>

void server_init(struct server *server) {
    server->display = wl_display_create();
    if (!server->display) {
        SERVER_FATAL("Failed to create Wayland display");
    }

    /* Init lists */
    wl_list_init(&server->surfaces);
    wl_list_init(&server->shm_pools);

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
        6, server, bind_compositor
    );

    if (!server->xdg_wm_base_global || !server->shm_global || !server->compositor_global) {
        SERVER_FATAL("Failed to create Wayland globals");
    }
}

void server_run(struct server *server) {
    SERVER_INFO("Wayland server is running");
    wl_display_run(server->display);
}

#define CLEANUP_WL_LIST_1(type, list) \
    do { \
        struct type *member, *member_tmp; \
        wl_list_for_each_safe(member, member_tmp, list, link) { \
            wl_list_remove(&member->link); \
            free(member); \
        } \
    } while(0)

#define CLEANUP_WL_LIST_2(type, list, callback) \
    do { \
        struct type *member, *member_tmp; \
        wl_list_for_each_safe(member, member_tmp, list, link) { \
            wl_list_remove(&member->link); \
            callback(member); \
            free(member); \
        } \
    } while(0)

#define CLEANUP_WL_LIST_HELPER(_1, _2, _3, NAME, ...) NAME
#define CLEANUP_WL_LIST(...) CLEANUP_WL_LIST_HELPER(__VA_ARGS__, CLEANUP_WL_LIST_2, CLEANUP_WL_LIST_1, )

void server_cleanup(struct server *server) {
    wl_display_destroy_clients(server->display);

    /* Cleanup surfaces */
    CLEANUP_WL_LIST(surface, &server->surfaces);

    /* Cleanup shm_pools */
    CLEANUP_WL_LIST(shm_pool, &server->shm_pools, destroy_shm_pool);

    if (server->display) {
        wl_display_destroy(server->display);
        server->display = NULL;
    }
}