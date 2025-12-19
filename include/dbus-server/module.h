#ifndef DBUS_MODULE_H
#define DBUS_MODULE_H

/* Method type */
typedef struct dbus_server_method {
    char *name;
    char *signature;       // Входные аргументы
    char *return_signature; // Выходные аргументы
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
char *module_generate_introspection_xml(DBUS_MODULE *module, char *object_path);

/* Operations with modules list */
DBUS_MODULE *module_create(char *name);
void module_destroy(DBUS_MODULE *module);

/* Add interface to module */
DBUS_INTERFACE *module_add_interface(DBUS_MODULE *module, char *iface_name);

/* Add method to interface */
DBUS_METHOD *interface_add_method(DBUS_INTERFACE *iface, char *method_name);


#endif