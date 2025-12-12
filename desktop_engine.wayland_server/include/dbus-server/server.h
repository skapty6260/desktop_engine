#ifndef DBUS_SERVER_H
#define DBUS_SERVER_H

#include <dbus/dbus.h>
#include <pthread.h>
#include <stdbool.h>

struct dbus_server {
    DBusConnection *connection;
    char *bus_name;
    bool is_running;
    pthread_t thread_id;
    pthread_mutex_t mutex;
};

struct dbus_server *dbus_create_server(char *bus_name);
void dbus_server_cleanup(struct dbus_server *server);
int dbus_start_main_loop(struct dbus_server *server);

#endif