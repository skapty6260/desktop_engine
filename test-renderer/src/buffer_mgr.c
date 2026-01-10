#include <buffer_mgr.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

BufferMgr_t *g_buffer_mgr = NULL;

static void update_buffer_from_fd(RenderBuffer_t *buffer, dbus_uint32_t width, dbus_uint32_t height, dbus_uint32_t stride, dbus_uint32_t format, const char *type_str, const char *format_str, int fd) {
    size_t buffer_size = 0;

    // Get file size
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat failed");
        return;
    }
    
    size_t file_size = st.st_size;
    if (buffer_size == 0) {
        buffer_size = file_size;
    }
    
    printf("File size: %zu bytes\n", file_size);

    // Mmap shm to memory
    void *mapped_data = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("mmap failed");
        return;
    }

    printf("SHM mapped at %p, size: %zu\n", mapped_data, file_size);
    
    // Cleanup previous data
    if (buffer->mmaped && buffer->data) {
        munmap(buffer->data, buffer->size);
        buffer->mmaped = false;
        buffer->data = NULL;
    } else if (buffer->data && !buffer->mmaped) {
        free(buffer->data);
        buffer->data = NULL;
    }

    // Update buffer struct
    buffer->data = mapped_data;
    buffer->size = file_size;
    buffer->capacity = file_size;
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride;
    // buffer->format = NULL;
    buffer->fd = fd;
    buffer->mmaped = true;
    buffer->dirty = true;
    
    printf("Buffer updated successfully:\n");
    printf("  Address: %p\n", buffer->data);
    printf("  Size: %zu bytes\n", buffer->size);
    printf("  Dimensions: %ux%u\n", buffer->width, buffer->height);
    printf("  Stride: %u bytes\n", buffer->stride);
    printf("  Format: %s\n", format_str);
    printf("  FD: %d\n", buffer->fd);
    printf("  Mmaped: %s\n", buffer->mmaped ? "yes" : "no");
    printf("  Dirty: %s\n", buffer->dirty ? "yes" : "no");
}

static DBusHandlerResult message_handler(DBusConnection *connection, DBusMessage *message, void *user_data) {    
    if (dbus_message_is_signal(message, 
        "org.skapty6260.DesktopEngine.Buffer", 
        "Updated")) {
        
        DBusError err;
        dbus_error_init(&err);
        
        DBusMessageIter iter;
        if (!dbus_message_iter_init(message, &iter)) {
            printf("Failed to get message iterator\n");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        
        // Проверяем тип первого аргумента (должна быть структура)
        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRUCT) {
            printf("Expected struct, got type %c\n", dbus_message_iter_get_arg_type(&iter));
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        
        DBusMessageIter struct_iter;
        dbus_message_iter_recurse(&iter, &struct_iter);
        dbus_uint32_t width = 0;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_UINT32) {
            dbus_message_iter_get_basic(&struct_iter, &width);
            dbus_message_iter_next(&struct_iter);
        }
        dbus_uint32_t height = 0;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_UINT32) {
            dbus_message_iter_get_basic(&struct_iter, &height);
            dbus_message_iter_next(&struct_iter);
        }
        dbus_uint32_t stride = 0;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_UINT32) {
            dbus_message_iter_get_basic(&struct_iter, &stride);
            dbus_message_iter_next(&struct_iter);
        }
        dbus_uint32_t format = 0;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_UINT32) {
            dbus_message_iter_get_basic(&struct_iter, &format);
            dbus_message_iter_next(&struct_iter);
        }
        const char *type_str = NULL;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&struct_iter, &type_str);
            dbus_message_iter_next(&struct_iter);
        }
        const char *format_str = NULL;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&struct_iter, &format_str);
            dbus_message_iter_next(&struct_iter);
        }
        int fd = -1;
        if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_UNIX_FD) {
            dbus_message_iter_get_basic(&struct_iter, &fd);
            dbus_message_iter_next(&struct_iter);
        }
        
        update_buffer_from_fd(g_buffer_mgr->buffers, width, height, stride, format, type_str, format_str, fd);
        
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

    printf("buffer fetcher worker cleanup\n");
    dbus_connection_remove_filter(conn, message_handler, NULL);
    dbus_connection_unref(conn);

    return NULL;
}

void create_buffermgr_thread() {
    g_buffer_mgr = calloc(1, sizeof(BufferMgr_t));
    g_buffer_mgr->running = true;

    // Initial buffer
    g_buffer_mgr->buffers = calloc(1, sizeof(RenderBuffer_t));

    pthread_create(&g_buffer_mgr->tid, NULL, buffer_fetcher_worker, NULL); //  2 arg is thread attrs (TODO), 4th is arguments for worker function in future
}

void stop_buffermgr_thread() {
    if (!g_buffer_mgr) return;
    
    g_buffer_mgr->running = false;
    
    pthread_join(g_buffer_mgr->tid, NULL);
    printf("buffermgr thread stopped\n");
}

void cleanup_buffermgr() {
    printf("buffermgr_cleanup called!\n");
    
    if (!g_buffer_mgr) return;
    
    if (g_buffer_mgr->running) {
        stop_buffermgr_thread();
    }
    
    if (g_buffer_mgr->buffers) {
        RenderBuffer_t *buffer = g_buffer_mgr->buffers;
        
        if (buffer->mmaped && buffer->data) {
            munmap(buffer->data, buffer->size);
            buffer->mmaped = false;
        } 
        else if (buffer->data && !buffer->mmaped) {
            free(buffer->data);
        }
        
        if (buffer->fd >= 0) {
            close(buffer->fd);
        }
        
        free(g_buffer_mgr->buffers);
    }
    
    free(g_buffer_mgr);
    g_buffer_mgr = NULL;
}