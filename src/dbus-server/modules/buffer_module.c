#include <dbus-server/modules/buffer_module.h>
#include <logger.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h> 

/* Convert pixel_format to string */
const char *pixel_format_to_string(enum pixel_format format) {
    switch (format) {
        case PIXEL_FORMAT_ARGB8888: return "ARGB8888";
        case PIXEL_FORMAT_XRGB8888: return "XRGB8888";
        default: return "UNKNOWN";
    }
}

/* Convert string to pixel_format */
enum pixel_format string_to_pixel_format(const char *str) {
    if (!str) return PIXEL_FORMAT_UNKNOWN;
    if (strcmp(str, "ARGB8888") == 0) return PIXEL_FORMAT_ARGB8888;
    if (strcmp(str, "XRGB8888") == 0) return PIXEL_FORMAT_XRGB8888;
    return PIXEL_FORMAT_UNKNOWN;
}


DBUS_MODULE *create_buffer_module(void) {
    DBUS_MODULE *module = module_create("Buffer_Broadcast");
    if (!module) {
        SERVER_ERROR("Failed to create buffer module");
        return NULL;
    }

    // Add interface
    DBUS_INTERFACE *iface = module_add_interface(module,
                                                "org.skapty6260.DesktopEngine.Buffer", 
                                                "/org/skapty6260/DesktopEngine/Buffer");
    if (!iface) {
        SERVER_ERROR("Failed to add interface to buffer module");
        module_destroy(module);
        return NULL;
    }

    // Add methods to the interface
    interface_add_method(iface, "Update", "s", "s", buffer_update_handler, NULL);
    interface_add_method(iface, "Get", "", "s", buffer_get_handler, NULL);
    interface_add_method(iface, "Set", "s", "s", buffer_set_handler, NULL);

    SERVER_DEBUG("Buffer module created successfully");
    return module;
}

DBusHandlerResult buffer_update_handler(DBusConnection *conn, DBusMessage *msg, void *user_data) {
    SERVER_DEBUG("Buffer update handler called");
    
    // TODO: Implement actual buffer update logic
    const char *response = "Buffer update received";
    
    DBusMessage *reply = dbus_message_new_method_return(msg);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult buffer_get_handler(DBusConnection *conn, DBusMessage *msg, void *user_data) {
    SERVER_DEBUG("Buffer get handler called");
    
    // TODO: Get actual buffer info
    const char *response = "Buffer info: width=0, height=0, format=UNKNOWN";
    
    DBusMessage *reply = dbus_message_new_method_return(msg);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult buffer_set_handler(DBusConnection *conn, DBusMessage *msg, void *user_data) {
    SERVER_DEBUG("Buffer set handler called");
    
    char *buffer_data = NULL;
    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &buffer_data, DBUS_TYPE_INVALID)) {
        DBusMessage *error = dbus_message_new_error(msg, 
            "org.skapty6260.DesktopEngine.Buffer_Broadcast.Error.InvalidArgs",
            "Invalid arguments");
        dbus_connection_send(conn, error, NULL);
        dbus_message_unref(error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    SERVER_DEBUG("Received buffer data: %s", buffer_data);
    
    // TODO: Process buffer data
    const char *response = "Buffer set successfully";
    
    DBusMessage *reply = dbus_message_new_method_return(msg);
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

/* Signal function */
void buffer_module_send_update_signal(DBusConnection *conn, const BufferInfo *info) {
    if (!conn || !info) {
        SERVER_ERROR("Invalid parameters for buffer_module_send_update_signal");
        return;
    }

    SERVER_DEBUG("=== SENDING BUFFER SIGNAL ===");
    SERVER_DEBUG("Connection: %p", conn);
    SERVER_DEBUG("Buffer info: %ux%u, stride=%u", info->width, info->height, info->stride);
    
    const char *signal_path = "/org/skapty6260/DesktopEngine/Buffer";
    
    SERVER_DEBUG("Creating signal on path: %s", signal_path);
    
    // Создаем сигнал D-Bus
    DBusMessage *signal = dbus_message_new_signal(
        signal_path,  // Путь объекта
        "org.skapty6260.DesktopEngine.Buffer",  // Интерфейс
        "Updated");  // Имя сигнала
    
    if (!signal) {
        SERVER_ERROR("Failed to create D-Bus signal message");
        return;
    }
    
    SERVER_DEBUG("Signal created successfully");
    
    // Готовим строку с информацией
    const char *type_str = "UNKNOWN";
    switch (info->type) {
        case WL_BUFFER_SHM: type_str = "SHM"; break;
        case WL_BUFFER_EGL: type_str = "EGL"; break;
        case WL_BUFFER_DMA_BUF: type_str = "DMA-BUF"; break;
        default: type_str = "UNKNOWN"; break;
    }
    
    // Создаем строку для отправки
    char info_str[512];
    snprintf(info_str, sizeof(info_str), 
             "Buffer updated: %ux%u, stride=%u, format=0x%x, size=%zu, type=%s",
             info->width, info->height, 
             info->stride,
             info->format,
             info->size,
             type_str);
    
    SERVER_DEBUG("Signal payload: %s", info_str);
    
    // Добавляем аргументы к сигналу
    const char *payload = info_str;
    if (!dbus_message_append_args(signal, 
                                 DBUS_TYPE_STRING, &payload,
                                 DBUS_TYPE_INVALID)) {
        SERVER_ERROR("Failed to append arguments to D-Bus signal");
        dbus_message_unref(signal);
        return;
    }
    
    SERVER_DEBUG("Arguments appended successfully");
    
    // Отправляем сигнал
    dbus_uint32_t serial = 0;
    if (!dbus_connection_send(conn, signal, &serial)) {
        SERVER_ERROR("Failed to send D-Bus signal");
    } else {
        SERVER_DEBUG("D-Bus signal sent, serial: %u", serial);
        // Важно: flush для немедленной отправки
        dbus_connection_flush(conn);
    }
    
    // Освобождаем сообщение
    dbus_message_unref(signal);
    
    SERVER_DEBUG("=== SIGNAL SEND COMPLETE ===");
}