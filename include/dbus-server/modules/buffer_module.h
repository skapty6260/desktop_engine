#ifndef DBUS_BUFFER_MODULE_H
#define DBUS_BUFFER_MODULE_H

#include <dbus-server/module-lib.h>
#include <dbus/dbus.h>
#include <stdint.h>
#include <stdbool.h>

#include <wayland/buffer.h>

/* Buffer transport data */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;      /* pixel_format как число */
    const char *format_str; /* Строковое представление формата */
    size_t size;
    bool has_data;
    enum wl_buffer_type type;  /* WL_BUFFER_SHM, WL_BUFFER_DMA_BUF, WL_BUFFER_EGL */
} BufferInfo;

/* Create module */
DBUS_MODULE *create_buffer_module(void);

/* Signals */
DBusHandlerResult buffer_update_handler(DBusConnection *conn, DBusMessage *msg, void *user_data);
DBusHandlerResult buffer_get_handler(DBusConnection *conn, DBusMessage *msg, void *user_data);
DBusHandlerResult buffer_set_handler(DBusConnection *conn, DBusMessage *msg, void *user_data);

/* Update signal (Internal) */
void buffer_module_send_update_signal(DBusConnection *conn, const BufferInfo *info);

/* Format convert */
const char *pixel_format_to_string(enum pixel_format format);
enum pixel_format string_to_pixel_format(const char *str);

#endif