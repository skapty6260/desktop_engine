#ifndef DESKTOPENGINE_DBUS_MODULE_LIB_H
#define DESKTOPENGINE_DBUS_MODULE_LIB_H

#include <dbus/dbus.h>

/* Callback type for method handlers */
typedef DBusHandlerResult (*DBUS_METHOD_HANDLER)(DBusConnection *connection, DBusMessage *message, void *user_data);

/* Method type (Linked list) */
typedef struct dbus_server_method {
    char *name;
    char *signature;
    char *return_signature;
    DBUS_METHOD_HANDLER handler;
    void *user_data;
    struct dbus_server_method *next;
} DBUS_METHOD;

/* Interface type (Linked list) */
typedef struct dbus_server_interface {
    char *name;
    DBUS_METHOD *methods;
    char *object_path;     // Путь объекта
    struct dbus_server_interface *next;
} DBUS_INTERFACE;

/* Module type (Linked list) */
typedef struct dbus_server_module {
    char *name;
    DBUS_INTERFACE *interfaces;
    struct dbus_server_module *next;
} DBUS_MODULE;

/* Generate introspect XML for module */
char *module_generate_introspection_xml(DBUS_MODULE *module, const char *object_path);

/* Operations with modules list */

/* Module operations */
DBUS_MODULE *module_create(char *name);
void module_destroy(DBUS_MODULE *module);

/* Interface operations */ // TODO: Add object path and other features
DBUS_INTERFACE *module_add_interface(DBUS_MODULE *module, char *iface_name);

/* Method operations */
DBUS_METHOD *interface_add_method(DBUS_INTERFACE *iface, char *method_name);
DBUS_METHOD *module_find_method(DBUS_MODULE *module, const char *interface_name, const char *method_name, const char *object_path);


#endif