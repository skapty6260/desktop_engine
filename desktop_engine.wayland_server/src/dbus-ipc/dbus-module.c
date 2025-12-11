#include <dbus-ipc/dbus-module.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool dbus_module_register(struct dbus_server *server, const char *name, const char *interface_name, const char *object_path, dbus_message_handler_t handler, void *user_data) {
    if (!server || !server->connection || !name || !object_path) {
        return false;
    }

    /* Check if there is already method with the same name or path */
    dbus_module_t *existing = server->modules;
    while (existing) {
        if (strcmp(existing->name, name) == 0 || strcmp(existing->object_path, object_path) == 0) {
            DBUS_ERROR("Module with name '%s' or path '%s' already exists", name, object_path);
            return false;
        }
        existing = existing->next;
    }

    /* Create new module */
    dbus_module_t *module = malloc(sizeof(dbus_module_t));
    if (!module) return false;

    module->name = strdup(name);
    module->interface_name = interface_name ? strdup(interface_name) : NULL;
    module->object_path = strdup(object_path);
    module->handler = handler;
    module->user_data = user_data;
    module->next = NULL;

    /* Add module to server modules list */
    if (!server->modules) {
        server->modules = module;
    } else { /* Add to the end of list */
        dbus_module_t *last = server->modules;
        while (last->next) {
            last = last->next;
        }
        last->next = module;
    }

    DBUS_INFO("Registered module: %s at %s", name, object_path);
    return true;
}

void dbus_module_unregister(struct dbus_server *server, const char *name) {
    if (!server || !server->modules || !name) {
        return;
    }

    dbus_module_t *prev = NULL;
    dbus_module_t *current = server->modules;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            /* Delete from list */
            if (prev) {
                prev->next = current->next;
            } else {
                server->modules = current->next;
            }
            
            /* Cancel object registration */
            if (server->connection) {
                dbus_connection_unregister_object_path(server->connection, current->object_path);
            }
            
            /* Cleanup module*/
            free(current->name);
            free(current->interface_name);
            free(current->object_path);
            free(current);
            
            DBUS_INFO("Unregistered module: %s", name);
            return;
        }

        prev = current;
        current = current->next;
    }
}