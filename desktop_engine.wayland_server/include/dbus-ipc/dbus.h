#ifndef DBUS_SERVER_H
#define DBUS_SERVER_H

#include <stdbool.h>
#include <wayland-server.h>

// Структура D-Bus сервера (неполный тип)
struct dbus_server;

// Создание и уничтожение
struct dbus_server *dbus_server_create(void);
void dbus_server_cleanup(struct dbus_server *server);

// Инициализация с интеграцией в Wayland
bool dbus_server_init(struct dbus_server *server, struct wl_display *display);

// Получение bus name для информации
const char *dbus_server_get_name(struct dbus_server *server);

#endif