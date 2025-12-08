// TODO: better cleanup and connection handling; Message filter
#include <dbus-ipc/dbus-core.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Main message handler (delegates messages to the modules) */
DBusHandlerResult core_message_handler(DBusConnection *connection, DBusMessage *message, void *user_data) {
    struct dbus_server *server = (struct dbus_server *)user_data;
    const char *object_path = dbus_message_get_path(message);
    const char *interface = dbus_message_get_interface(message);

    if (!object_path || !interface) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    /* Search for module registered for this path */
    dbus_module_t *module = server->modules;
    while (module) {
        if (strcmp(module->object_path, object_path) == 0 ||
            (strcmp(interface, module->interface_name) == 0 && 
             strcmp(object_path, module->object_path) == 0)) {
            
            /* Delegate handling to the module */
            if (module->handler) {
                return module->handler(connection, message, module->user_data);
            }
            break;
        }
        module = module->next;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool dbus_core_init_connection(struct dbus_server *server) {
    if (!server) return false;

    DBusError err;
    dbus_error_init(&err);

    /* Connect to the session bus */
    server->connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        DBUS_ERROR("D-Bus connection error: %s", err.message);
        dbus_error_free(&err);
        return false;
    }

    /* Requesting dbus service name */
    int ret = dbus_bus_request_name(server->connection,
                                    BUS_NAME,
                                    DBUS_NAME_FLAG_REPLACE_EXISTING,
                                    &err);
    if (dbus_error_is_set(&err) || ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        if (dbus_error_is_set(&err)) {
            DBUS_ERROR("D-Bus name error: %s", err.message);
            dbus_error_free(&err);
        } else {
            DBUS_ERROR("Failed to acquire name %s (ret=%d)", BUS_NAME, ret);
        }
        dbus_connection_unref(server->connection);
        server->connection = NULL;
        return false;
    }

    /* Register core message handler */
    DBusObjectPathVTable vtable = {
        .unregister_function = NULL,
        .message_function = core_message_handler,
        .dbus_internal_pad1 = NULL,
        .dbus_internal_pad2 = NULL,
        .dbus_internal_pad3 = NULL,
        .dbus_internal_pad4 = NULL
    };

    /* Register root path for message handling */
    if (!dbus_connection_register_object_path(server->connection,
                                              "/",
                                              &vtable,
                                              server)) {
        DBUS_ERROR("Failed to register D-Bus objectr path");
        dbus_connection_unref(server->connection);
        server->connection = NULL;
        return false;
    }

    /* Acquiring dbus fd */
    if (!dbus_connection_get_unix_fd(server->connection, &server->dbus_fd)) {
        DBUS_ERROR("Failed to get D-Bus file descriptor");
        dbus_connection_unref(server->connection);
        server->connection = NULL;
        return false;
    }

    server->initialized = true;
    DBUS_INFO("D-Bus core initialized with bus name: %s", BUS_NAME);

    return true;
}

void dbus_core_cleanup(struct dbus_server *server) {
    if (!server) return;

    /* Cleanup all modules */
    dbus_module_t *module = server->modules;
    while (module) {
        dbus_module_t *next = module->next;

        /* Cancel module registration */
        if (server->connection && module->object_path) {
            dbus_connection_unregister_object_path(server->connection, module->object_path);
        }

        free(module->name);
        free(module->interface_name);
        free(module->object_path);
        free(module);

        module = next;
    }
    server->modules = NULL;

    /* Close connection */
    if (server->connection) {
        DBusError err;
        dbus_error_init(&err);
 
        /* Release bus name */
        dbus_bus_release_name(server->connection, BUS_NAME, &err);
        if (dbus_error_is_set(&err)) {
            DBUS_ERROR("Failed to release name: %s", err.message);
            dbus_error_free(&err);
        }

        // dbus_connection_close(server->connection);
        dbus_connection_unref(server->connection);
        server->connection = NULL;
    }

    server->initialized = false;
    server->dbus_fd = -1;
    free(server);
    DBUS_DEBUG("D-Bus cleaned");
}