#ifndef DBUS_SERVER_H
#define DBUS_SERVER_H

#include <dbus/dbus.h>
#include <pthread.h>
#include <stdbool.h>

#include <dbus-server/module.h>

typedef struct dbus_server_module DBUS_MODULE;

struct dbus_server {
    DBusConnection *connection;
    char *bus_name;
    bool is_running;
    pthread_t thread_id;
    pthread_mutex_t mutex;
    int wakeup_pipe[2];
    struct pollfd *pollfds;
    int pollfd_count;
    DBUS_MODULE *modules; // list of modules (HEAD)
    DBUS_MODULE *modules_tail; // modules list tail
};

/* Server modules operations */
void dbus_server_add_module(struct dbus_server *server, DBUS_MODULE *module);
DBUS_MODULE *dbus_server_find_module(struct dbus_server *server, char *name);
void dbus_server_remove_module(struct dbus_server *server, char *name);
void dbus_server_remove_all_modules(struct dbus_server *server);

/* Main server operations */
struct dbus_server *dbus_create_server(char *bus_name);
void dbus_server_cleanup(struct dbus_server *server);
int dbus_start_main_loop(struct dbus_server *server);
void release_bus_name(DBusConnection *conn, const char *name);

#endif