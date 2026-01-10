#include <buffer_mgr.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

BufferMgr_t *g_buffer_mgr = NULL;

static DBusHandlerResult message_handler(DBusConnection *connection, DBusMessage *message, void *user_data) {
    if (dbus_message_is_signal(message, 
        "org.skapty6260.DesktopEngine.Buffer", 
        "Updated")) {
        
        DBusError err;
        dbus_error_init(&err);
        
        char *info = NULL;
        if (dbus_message_get_args(message, &err, 
                                 DBUS_TYPE_STRING, &info,
                                 DBUS_TYPE_INVALID)) {
            printf("Buffer Updated: %s\n", info);
        } else {
            fprintf(stderr, "Failed to parse signal: %s\n", err.message);
            dbus_error_free(&err);
        }
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void* buffer_fetcher_worker(void* arg) {
    DBusError err;
    DBusConnection *conn;
    const char *unique_name;
    
    dbus_error_init(&err);

    // Connect to session bus
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error: %s\n", err.message);
        dbus_error_free(&err);
        return NULL;
    }

    // Store connection for cleanup
    g_buffer_mgr->conn = conn;

    // Get bus name
    unique_name = dbus_bus_get_unique_name(conn);
    if (unique_name) {
        printf("Connected to DBus with unique name: %s\n", unique_name);
    }

    // Request name
    int ret = dbus_bus_request_name(conn, "org.skapty6260.DesktopEngine.BufferMonitor", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error: %s\n", err.message);
        dbus_error_free(&err);
        return NULL;
    }

    if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        printf("Successfully acquired bus name\n");
    } else {
        printf("Bus name request result: %d\n", ret);
    }

    // Add signal filter
    dbus_bus_add_match(conn, 
        "type='signal',"
        "interface='org.skapty6260.DesktopEngine.Buffer',"
        "path='/org/skapty6260/DesktopEngine/Buffer',"
        "sender='org.skapty6260.DesktopEngine'",
        &err);
    
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Match Error: %s\n", err.message);
        dbus_error_free(&err);
        return NULL;
    }
    
    printf("Match rule added successfully\n");

    // Список всех правил для проверки (убрана, чтобы не вызывала ошибку)
    // ...
    
    printf("\nMonitoring buffer updates...\n");

    // Проверяем конкретный сервис (убрана, чтобы не вызывала ошибку)
    // ...
    
    // Add filter with callback
    if (!dbus_connection_add_filter(conn, message_handler, NULL, NULL)) {
        fprintf(stderr, "Failed to add filter\n");
        return NULL;
    }

    // Main handling loop
    printf("Entering main loop...\n");

    // TODO: async handling
    while (g_buffer_mgr->running) {
        dbus_connection_read_write_dispatch(conn, 1000); // 1 секунда таймаут
    }

    printf("buffer fetcher worker cleanup");
    dbus_connection_remove_filter(conn, message_handler, NULL);
    dbus_connection_unref(conn);

    return NULL;
}

void create_buffermgr_thread() {
    g_buffer_mgr = calloc(1, sizeof(BufferMgr_t));
    g_buffer_mgr->running = true;

    pthread_create(&g_buffer_mgr->tid, NULL, buffer_fetcher_worker, NULL); //  2 arg is thread attrs (TODO), 4th is arguments for worker function in future
}

void stop_buffermgr_thread() {
    g_buffer_mgr->running = false;
    
    // Wait for thread to finish
    pthread_join(g_buffer_mgr->tid, NULL);
    printf("buffermgr thread stopped\n");
}

void cleanup_buffermgr() {
    printf("buffermgr_cleanup called!");
    if (g_buffer_mgr->running) {
        stop_buffermgr_thread();
    }

    free(g_buffer_mgr);
    g_buffer_mgr = NULL;
}