#include <dbus/dbus.h> // dbus lib
#include <dbus-ipc/dbus.h> // dbus-ipc header file (My implementation)
#include <stdlib.h>
#include <stdio.h>
#include <wayland-server.h>

struct dbus_server {
    struct wl_display *wl_display;
    struct wl_event_source *wl_fd_source;
    DBusConnection *conn;
    int dbus_fd;
    bool initialized;
};

// Callback для Wayland event loop
static int dbus_fd_callback(int fd, uint32_t mask, void *data) {
    struct dbus_server *server = (struct dbus_server *)data;
    
    // Просто читаем и диспатчим сообщения без обработки
    if (server->conn) {
        dbus_connection_read_write_dispatch(server->conn, 0);
    }
    
    return 0;
}

// Пустой обработчик сообщений (ничего не делает)
static DBusHandlerResult null_message_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data) {
    // Ничего не делаем, просто возвращаем NOT_YET_HANDLED
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

struct dbus_server *dbus_server_create(void) {
    return calloc(1, sizeof(struct dbus_server));
}

bool dbus_server_init(struct dbus_server *server, struct wl_display *display) {
    if (!server || !display) {
        return false;
    }
    
    DBusError err;
    dbus_error_init(&err);
    
    // Подключаемся к session bus
    server->conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "D-Bus error: %s\n", err.message);
        dbus_error_free(&err);
        return false;
    }
    
    // Запрашиваем простое имя
    int ret = dbus_bus_request_name(server->conn,
                                   "org.wayland.minimal",
                                   DBUS_NAME_FLAG_REPLACE_EXISTING,
                                   &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "D-Bus name error: %s\n", err.message);
        dbus_error_free(&err);
        dbus_connection_unref(server->conn);
        return false;
    }
    
    // Регистрируем пустой объектный путь
    DBusObjectPathVTable vtable = {
        .unregister_function = NULL,
        .message_function = null_message_handler,
        .dbus_internal_pad1 = NULL,
        .dbus_internal_pad2 = NULL,
        .dbus_internal_pad3 = NULL,
        .dbus_internal_pad4 = NULL
    };
    
    if (!dbus_connection_register_object_path(server->conn,
                                              "/org/wayland/minimal",
                                              &vtable,
                                              server)) {
        fprintf(stderr, "Failed to register D-Bus object path\n");
        dbus_connection_unref(server->conn);
        return false;
    }
    
    // Получаем файловый дескриптор
    dbus_connection_get_unix_fd(server->conn, &server->dbus_fd);
    if (server->dbus_fd < 0) {
        fprintf(stderr, "Failed to get D-Bus file descriptor\n");
        dbus_connection_unref(server->conn);
        return false;
    }
    
    // Сохраняем Wayland display
    server->wl_display = display;
    
    // Добавляем FD в Wayland event loop
    server->wl_fd_source = wl_event_loop_add_fd(
        wl_display_get_event_loop(display),
        server->dbus_fd,
        WL_EVENT_READABLE,
        dbus_fd_callback,
        server);
    
    if (!server->wl_fd_source) {
        fprintf(stderr, "Failed to add D-Bus fd to Wayland loop\n");
        dbus_connection_unref(server->conn);
        return false;
    }
    
    server->initialized = true;
    printf("D-Bus server started (minimal, no handlers)\n");
    
    return true;
}

const char *dbus_server_get_name(struct dbus_server *server) {
    return "org.wayland.minimal";
}

void dbus_server_cleanup(struct dbus_server *server) {
    if (!server) return;
    
    // Удаляем из Wayland event loop
    if (server->wl_fd_source) {
        wl_event_source_remove(server->wl_fd_source);
    }
    
    // Закрываем D-Bus соединение
    if (server->conn) {
        dbus_connection_close(server->conn);
        dbus_connection_unref(server->conn);
    }
    
    free(server);
}