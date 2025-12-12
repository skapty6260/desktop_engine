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
    if (!conn || !name) return;

    DBusError err;
    int ret;

    dbus_error_init(&err);

    ret = dbus_bus_request_name(conn, name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        DBUS_ERROR("Failed to request bus name");
        dbus_error_free(&err);
        return -1;
    }

    switch (ret) {
        case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
            DBUS_INFO("Successfully acquired bus name '%s' as primary owner", name);
            return 0;
            
        case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
            DBUS_WARN("Bus name '%s' is already owned, added to queue", name);
            return 0;
            
        case DBUS_REQUEST_NAME_REPLY_EXISTS:
            DBUS_ERROR("Bus name '%s' already exists and cannot be acquired", name);
            return -1;
            
        case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
            DBUS_INFO("Already own the bus name '%s'", name);
            return 0;
            
        default:
            DBUS_ERROR("Unknown return value from dbus_bus_request_name: %d", ret);
            return -1;
    }
}

static void release_bus_name(DBusConnection *conn, const char *name) {
    if (!conn || !name) return;
    
    DBusError err;
    dbus_error_init(&err);

    dbus_bus_release_name(conn, name, &err);
    
    if (dbus_error_is_set(&err)) {
        DBUS_ERROR("Failed to release bus name '%s': %s", name, err.message);
        dbus_error_free(&err);
    } else {
        DBUS_DEBUG("Released bus name: %s", name);
    }
}

struct dbus_server *dbus_create_server(char *bus_name) {
    struct dbus_server *server = calloc(1, sizeof(struct dbus_server));

    server->connection = create_dbus_connection();
    if (!server->connection) {
        DBUS_ERROR("Failed to get session bus connection");
        return NULL;
    }

    if (request_bus_name(server->connection, bus_name) != 0) {
        dbus_connection_unref(server->connection);
        return NULL;
    }
    server->bus_name = strdup(bus_name);

    DBUS_INFO("D-Bus server created with name: %s", bus_name);
    return server;
}

void dbus_server_cleanup(struct dbus_server *server) {
    if (!server) return;

    release_bus_name(server->connection, server->bus_name);
    dbus_connection_unref(server->connection);
}