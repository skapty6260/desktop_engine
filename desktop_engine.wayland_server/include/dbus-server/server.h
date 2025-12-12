#ifndef DBUS_SERVER_H
#define DBUS_SERVER_H

#include <dbus/dbus.h>
#include <string.h>

struct dbus_server {
    DBusConnection *connection;
    char *bus_name;
};

struct dbus_server *dbus_create_server(const char *bus_name);

#endif