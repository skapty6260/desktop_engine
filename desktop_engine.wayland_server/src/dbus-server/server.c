/* This should create struct, init dbus connection, provide cleanup */
#include <dbus-server/server.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static DBusConnection *create_dbus_connection() {
    DBusError err;
    DBusConnection *conn;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        DBUS_ERROR("D-Bus connection failed: %s", err.message);
        dbus_error_free(&err);
        return NULL;
    }

    return conn;
}

static int request_bus_name(DBusConnection *conn, char *name) {
    DBusError err;
    int ret;

    dbus_error_init(&err);

    ret = dbus_bus_request_name(conn, name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        DBUS_ERROR("Failed to request bus name");
        dbus_error_free(&err);
        return -1;
    }

    return ret;
}

struct dbus_server *dbus_create_server(char *bus_name) {
    struct dbus_server *server = calloc(1, sizeof(struct dbus_server));

    server->connection = create_dbus_connection();
    if (!server->connection) {
        DBUS_ERROR("Failed to get session bus connection");
        return NULL;
    }

    if (request_bus_name(server->connection, bus_name) != 0) {
        return NULL;
    }
    server->bus_name = bus_name;

    DBUS_INFO("D-Bus server created with name: %s", bus_name);
    return server;
}

void dbus_server_cleanup(struct dbus_server *server) {
    if (!server) return;

    dbus_connection_unref(server->connection);
}