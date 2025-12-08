#ifndef DBUS_CORE_H
#define DBUS_CORE_H

#define BUS_NAME "org.skapty6260.DesktopEngine"

#include <stdbool.h>
#include <dbus/dbus.h>

typedef DBusHandlerResult (*dbus_message_handler_t)(DBusConnection *, DBusMessage *, void*);

typedef struct dbus_module {
    char *name;
    char *interface_name;
    char *object_path;
    dbus_message_handler_t handler;
    void *user_data;
    struct dbus_module *next;
} dbus_module_t;

struct dbus_server {
    DBusConnection *connection;
    int dbus_fd;
    bool initialized;
    dbus_module_t *modules;
};

bool dbus_core_init_connection(struct dbus_server *server);
void dbus_core_cleanup(struct dbus_server *server);

#endif