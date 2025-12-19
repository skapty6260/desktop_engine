/* This should create struct, init dbus connection, provide cleanup */
#include <dbus-server/server.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

/* Init dbus connection */
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

/* Request bus name */
static int request_bus_name(DBusConnection *conn, char *name) {
    if (!conn || !name) return -1;

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

/* Handle introspect call */
static void handle_introspect(struct dbus_server *server, DBusMessage *msg, const char *path) {
    char *introspection_data = NULL;

    /* Generate dynamic XML */
    pthread_mutex_lock(&server->mutex);

    if (strcmp(path, "/") == 0) {
        /* Root path - generate list of all objects */
        introspection_data = strdup(
            "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node>\n"
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
            "    <method name=\"Introspect\">\n"
            "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
            "    </method>\n"
            "  </interface>\n"
            "</node>\n"
        );
    } else {
        /* Search for module by path */
        DBUS_MODULE *module = server->modules;
        while (module) {
            if (strstr(path, module->name) != NULL) {
                introspection_data = module_generate_introspection_xml(module, path);
                break;
            }
            module = module->next;
        }
    }
        
    pthread_mutex_unlock(&server->mutex);
        
    /* If found nothing, return default data */
    if (!introspection_data) {
        introspection_data = strdup(
            "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node>\n"
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
            "    <method name=\"Introspect\">\n"
            "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
            "    </method>\n"
            "  </interface>\n"
            "</node>\n"
        );
    }
        
    /* Send reply */
    DBusMessage *reply = dbus_message_new_method_return(msg);
    if (reply && introspection_data) {
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_data, DBUS_TYPE_INVALID);
        dbus_connection_send(server->connection, reply, NULL);
    } else {
        DBUS_ERROR("Failed to create Introspect reply");
    }
        
    if (reply) dbus_message_unref(reply);
    if (introspection_data) free(introspection_data);
}

/* Handle method call */
static void handle_method_call(struct dbus_server *server, DBusMessage *msg, const char *interface, const char *method_name, const char *path) {
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    
    pthread_mutex_lock(&server->mutex);

    /* Search through all modules for matching method */
    DBUS_MODULE *module = server->modules;
    while (module && result == DBUS_HANDLER_RESULT_NOT_YET_HANDLED) {
        DBUS_METHOD *method = module_find_method(module, interface, method_name, path);
        if (method && method->handler) {
            /* Found handler - unlock mutex while calling handler */
            pthread_mutex_unlock(&server->mutex);
            
            result = method->handler(server->connection, msg, method->user_data);
            
            /* Re-lock if we need to continue searching */
            if (result == DBUS_HANDLER_RESULT_NOT_YET_HANDLED) {
                pthread_mutex_lock(&server->mutex);
            }
        } else {
            module = module->next;
        }
    }

    pthread_mutex_unlock(&server->mutex);

    /* If no handler found, send error */
    if (result == DBUS_HANDLER_RESULT_NOT_YET_HANDLED) {
        DBUS_DEBUG("No handler found for %s.%s on path %s", interface, method_name, path);
        
        /* Send UnknownMethod error */
        DBusMessage *reply = dbus_message_new_error(
            msg,
            DBUS_ERROR_UNKNOWN_METHOD,
            "Method does not exist"
        );
        if (reply) {
            dbus_connection_send(server->connection, reply, NULL);
            dbus_message_unref(reply);
        }
    }
}

/* Process dbus message */
static void proccess_message(struct dbus_server *server, DBusMessage *msg) {
    const char *path = dbus_message_get_path(msg);
    const char *interface = dbus_message_get_interface(msg);
    const char *method_name = dbus_message_get_member(msg);
    
    DBUS_DEBUG("Received message: path=%s, interface=%s, method=%s", path ? path : "/", interface ? interface : "(null)", method_name ? method_name : "(null)");

    /* Handle introspect */
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        handle_introspect(server, msg, path);
        dbus_message_unref(msg);
        return;
    }

    /* Check if it's a method call */
    if (!interface || !method_name) {
        DBUS_DEBUG("Not a method call, ignoring");
        dbus_message_unref(msg);
        return;
    }

    handle_method_call(server, msg, interface, method_name, path);

    dbus_message_unref(msg);
}

/* Release bus name */
void release_bus_name(DBusConnection *conn, const char *name) {
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

/* Create dbus event loop thread */
static void *dbus_main_loop_thread(void *arg) {
    struct dbus_server *server = (struct dbus_server *)arg;

    if (!server || !server->connection) {
        DBUS_ERROR("Invalid server parameter in thread");
        return NULL;
    }

    DBUS_INFO("D-Bus async main loop thread started");

    /* Set connection to non-blocking */
    dbus_connection_set_exit_on_disconnect(server->connection, FALSE);

    /* Enable async dispatch (This tells D-Bus to queue messages instead of dispatching immediately) */
    dbus_connection_set_dispatch_status_function(server->connection, NULL, NULL, NULL);
    
    // TODO: Filter
    // dbus_connection_add_filter(server->connection, NULL, NULL, NULL);

    while(1) {
        pthread_mutex_lock(&server->mutex);
        bool running = server->is_running;
        pthread_mutex_unlock(&server->mutex);

        if (!running) break;

        // 4. Non-blocking read/write (timeout = 10ms) best use 1ms
        // This is async because it returns immediately if no data
        if (!dbus_connection_read_write_dispatch(server->connection, 10)) {
            DBUS_ERROR("Failed to read/write to D-Bus connection");
            break;
        }

        // 5. Process ONE message at a time (non-blocking)
        DBusMessage *msg = dbus_connection_pop_message(server->connection);
        if (msg) {
            proccess_message(server, msg);
        } else {
            sleep(0.1); // No message, sleep briefly to avoid busy-waiting (100ms) better to set one ms
        }
    }

    /* Mark thread as finished */
    pthread_mutex_lock(&server->mutex);
    server->thread_id = 0;
    pthread_mutex_unlock(&server->mutex);

    return NULL;
}

/* Stop dbus event loop */
static int dbus_stop_main_loop(struct dbus_server *server) {
    if (!server) {
        return -1;
    }
    
    pthread_mutex_lock(&server->mutex);
    if (!server->is_running) {
        pthread_mutex_unlock(&server->mutex);
        return 0;
    }

    /* Check if thread actually started */
    if (server->thread_id == 0) {
        server->is_running = false;
        pthread_mutex_unlock(&server->mutex);
        DBUS_INFO("D-Bus thread hasn't started yet, marking as stopped");
        return 0;
    }
    
    server->is_running = false;
    pthread_mutex_unlock(&server->mutex);
    
    DBUS_INFO("Stopping D-Bus main loop...");
    
    int max_wait_s = 3;
    int wait_step_s = 0.1;

    for (int waited = 0; waited < max_wait_s; waited += wait_step_s) {
        pthread_mutex_lock(&server->mutex);
        bool thread_done = (server->thread_id == 0);
        pthread_mutex_unlock(&server->mutex);
        
        if (thread_done) {
            DBUS_INFO("D-Bus thread exited gracefully");
            return 0;
        }
        
        sleep(wait_step_s);
    }
    
    return -1;
}

/* Add module to list end */
void dbus_server_add_module(struct dbus_server *server, DBUS_MODULE *module) {
    if (!server || !module) return;

    pthread_mutex_lock(&server->mutex);
    
    module->next = NULL;

    if (!server->modules) {
        // First module
        server->modules = module;
        server->modules_tail = module;
    } else {
        server->modules_tail->next = module;
        server->modules_tail = module;
    }

    pthread_mutex_unlock(&server->mutex);
}

/* Search for module by name */
DBUS_MODULE *dbus_server_find_module(struct dbus_server *server, char *name) {
    if (!server || !name) return NULL;
    
    pthread_mutex_lock(&server->mutex);
    
    DBUS_MODULE *current = server->modules;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            pthread_mutex_unlock(&server->mutex);
            return current;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->mutex);
    return NULL;
}

/* Remove module from list and destroy */
void dbus_server_remove_module(struct dbus_server *server, char *name) {
    if (!server || !name) return;
    
    pthread_mutex_lock(&server->mutex);
    
    DBUS_MODULE *current = server->modules;
    DBUS_MODULE *prev = NULL;
    
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                server->modules = current->next;
            }
            
            if (current == server->modules_tail) {
                server->modules_tail = prev;
            }
            
            module_destroy(current);
            pthread_mutex_unlock(&server->mutex);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->mutex);
}

/* Destroy all modules */
void dbus_server_remove_all_modules(struct dbus_server *server) {
    if (!server) return;
    
    pthread_mutex_lock(&server->mutex);
    
    DBUS_MODULE *current = server->modules;
    while (current) {
        DBUS_MODULE *next = current->next;
        module_destroy(current);
        current = next;
    }
    
    server->modules = NULL;
    server->modules_tail = NULL;
    
    pthread_mutex_unlock(&server->mutex);
}

/* Server constructor */
struct dbus_server *dbus_create_server(char *bus_name) {
    struct dbus_server *server = calloc(1, sizeof(struct dbus_server));
    if (!server) {
        DBUS_ERROR("Failed to calloc dbus_server");
        return NULL;
    }

    if (pthread_mutex_init(&server->mutex, NULL) != 0) {
        DBUS_ERROR("Failed to initialize mutext");
        free(server);
        return NULL;
    }

    server->connection = create_dbus_connection();
    if (!server->connection) {
        DBUS_ERROR("Failed to get session bus connection");
        free(server);
        return NULL;
    }

    if (request_bus_name(server->connection, bus_name) != 0) {
        dbus_connection_unref(server->connection);
        free(server);
        return NULL;
    }
    server->bus_name = strdup(bus_name);

    server->thread_id = 0;
    server->is_running = false;
    server->modules = NULL;
    server->modules_tail = NULL;

    return server;
}

/* Run main loop (Create async thread, set server running state)*/
int dbus_start_main_loop(struct dbus_server *server) {
    if (!server || !server->connection) {
        DBUS_ERROR("Can't start dbus main loop: server not connected");
        return -1;
    }

    pthread_mutex_lock(&server->mutex);
    if (server->is_running) {
        pthread_mutex_unlock(&server->mutex);
        DBUS_INFO("Main loop is already running");
        return 0;
    }

    server->is_running = true;
    pthread_mutex_unlock(&server->mutex);

    /* Create thread */
    int result = pthread_create(&server->thread_id, NULL, dbus_main_loop_thread, server);
    if (result != 0) {
        DBUS_ERROR("Failed to create D-Bus thread: %s", strerror(result));
        pthread_mutex_lock(&server->mutex);
        server->is_running = false;
        pthread_mutex_unlock(&server->mutex);
        return -1;
    }

    /* Detach the thread so it cleans up automatically */
    pthread_detach(server->thread_id);

    DBUS_INFO("D-Bus main loop started in separate thread: %lu", (unsigned long)server->thread_id);
    return 0;
}

/* Server cleanup */
void dbus_server_cleanup(struct dbus_server *server) {
    if (!server) return;

    dbus_stop_main_loop(server);
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&server->mutex);
        bool thread_done = (server->thread_id == 0);
        pthread_mutex_unlock(&server->mutex);
            
        if (thread_done) break;
            
        DBUS_DEBUG("Waiting for D-Bus thread to exit...");
        sleep(1);
    }

    if (server->bus_name) {
        release_bus_name(server->connection, server->bus_name);
        free(server->bus_name);
        server->bus_name = NULL;
    }

    if (server->connection) {
        dbus_connection_unref(server->connection);
        server->connection = NULL;
    }
    
    pthread_mutex_destroy(&server->mutex);
    free(server);

    DBUS_INFO("D-Bus server cleaned up");
}