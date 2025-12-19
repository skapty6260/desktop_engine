#include <dbus-server/module-lib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* String concat helper */
char *str_append(char *dest, const char *src) {
    if (!src) return dest;
    
    size_t dest_len = dest ? strlen(dest) : 0;
    size_t src_len = strlen(src);
    
    char *new_str = realloc(dest, dest_len + src_len + 1);
    if (!new_str) {
        free(dest);
        return NULL;
    }
    
    memcpy(new_str + dest_len, src, src_len + 1);
    return new_str;
}

/* Generate interface introspection (Just for Refactor) */
static char *generate_interface_introspection(DBUS_INTERFACE *iface, const char *object_path) {
    char *xml = NULL;

    // Validate object path
    if (!object_path || (iface->object_path && strcmp(iface->object_path, object_path) == 0)) {
        xml = str_append(xml, "  <interface name=\"");
        xml = str_append(xml, iface->name);
        xml = str_append(xml, "\">\n");

        /* Методы интерфейса */
        DBUS_METHOD *method = iface->methods;
        while (method) {
            xml = str_append(xml, "    <method name=\"");
            xml = str_append(xml, method->name);
            xml = str_append(xml, "\">\n");
                    
            /* Входные аргументы (in) */
            if (method->signature && method->signature[0] != '\0') {
                /* Парсим сигнатуру и создаем аргументы */
                // TODO: Реализовать парсинг сигнатуры D-Bus
                xml = str_append(xml, "      <arg name=\"value\" direction=\"in\" type=\"");
                xml = str_append(xml, method->signature);
                xml = str_append(xml, "\"/>\n");
            }
                    
            /* Выходные аргументы (out) */
            if (method->return_signature && method->return_signature[0] != '\0') {
                xml = str_append(xml, "      <arg name=\"result\" direction=\"out\" type=\"");
                xml = str_append(xml, method->return_signature);
                xml = str_append(xml, "\"/>\n");
            }
                    
            xml = str_append(xml, "    </method>\n");
            method = method->next;
        }
                
        xml = str_append(xml, "  </interface>\n");
    }

    return xml;
}

/* Generate XML Introspect for module */
char *module_generate_introspection_xml(DBUS_MODULE *module, const char *object_path) {
    char *xml = NULL;

    /* Header XML */
    xml = str_append(xml, "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n");
    xml = str_append(xml, "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n");
    xml = str_append(xml, "<node>\n");

    /* Standart interface Introspectable */
    xml = str_append(xml, "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n");
    xml = str_append(xml, "    <method name=\"Introspect\">\n");
    xml = str_append(xml, "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n");
    xml = str_append(xml, "    </method>\n");
    xml = str_append(xml, "  </interface>\n");

    /* Module user interfaces */
    if (module && module->interfaces) {
        DBUS_INTERFACE *iface = module->interfaces;
        while (iface) {
            char *iface_xml = generate_interface_introspection(iface, object_path);
            xml = str_append(xml, iface_xml);
            iface = iface->next;
        }
    }

    xml = str_append(xml, "</node>\n");
    return xml;
}

/* Create method helper */
static DBUS_METHOD *method_create(const char *name) {
    if (!name) return NULL;
    
    DBUS_METHOD *method = calloc(1, sizeof(DBUS_METHOD));
    if (!method) return NULL;
    
    method->name = strdup(name);
    if (!method->name) {
        free(method);
        return NULL;
    }
    
    method->handler = NULL;
    method->user_data = NULL;
    method->next = NULL;
    method->signature = NULL;
    method->return_signature = NULL;

    return method;
}

/* Destroy method*/
static void method_destroy(DBUS_METHOD *method) {
    if (!method) return;
    
    free(method->name);
    free(method->signature);
    free(method->return_signature);
    free(method);
}

/* Destroy methods chain */
static void methods_chain_destroy(DBUS_METHOD *method) {
    while (method) {
        DBUS_METHOD *next = method->next;
        method_destroy(method);
        method = next;
    }
}

/* Create interface helper */
static DBUS_INTERFACE *interface_create(const char *name) {
    if (!name) return NULL;
    
    DBUS_INTERFACE *iface = calloc(1, sizeof(DBUS_INTERFACE));
    if (!iface) return NULL;
    
    iface->name = strdup(name);
    if (!iface->name) {
        free(iface);
        return NULL;
    }
    
    iface->object_path = NULL;
    iface->methods = NULL;
    iface->next = NULL;
    return iface;
}

/* Destroy interface and all it's methods */
static void interface_destroy(DBUS_INTERFACE *iface) {
    if (!iface) return;
    
    free(iface->name);
    methods_chain_destroy(iface->methods);
    free(iface);
}

/* Destroy interface chain */
static void interface_chain_destroy(DBUS_INTERFACE *iface) {
    while (iface) {
        DBUS_INTERFACE *next = iface->next;
        interface_destroy(iface);
        iface = next;
    }
}

/* Module constructor */
DBUS_MODULE *module_create(char *name) {
    if (!name) return NULL;
    
    DBUS_MODULE *module = calloc(1, sizeof(DBUS_MODULE));
    if (!module) return NULL;
    
    module->name = strdup(name);
    if (!module->name) {
        free(module);
        return NULL;
    }
    
    module->interfaces = NULL;
    module->next = NULL;
    return module;
}

/* Module destructor (Destroy module, interfaces, methods) */
void module_destroy(DBUS_MODULE *module) {
    if (!module) return;
    
    free(module->name);
    interface_chain_destroy(module->interfaces);
    free(module);
}

/* Add interface to module */
DBUS_INTERFACE *module_add_interface(DBUS_MODULE *module, char *iface_name, char *object_path) {
    if (!module || !iface_name || !object_path) return NULL;
    
    DBUS_INTERFACE *new_iface = interface_create(iface_name);
    if (!new_iface) return NULL;
    
    /* Set object path */
    new_iface->object_path = strdup(object_path);
    if (!new_iface->object_path) {
        free(new_iface->name);
        free(new_iface);
        return NULL;
    }
    
    /* Add to list HEAD */
    new_iface->next = module->interfaces;
    module->interfaces = new_iface;
    
    return new_iface;
}

/* Add method to interface */
DBUS_METHOD *interface_add_method(DBUS_INTERFACE *iface, char *method_name, char *signature, char *return_signature, DBUS_METHOD_HANDLER handler, void *user_data) {
    if (!iface || !method_name) return NULL;
    
    DBUS_METHOD *new_method = method_create(method_name);
    if (!new_method) return NULL;
    
    /* Set method properties */
    new_method->signature = signature ? strdup(signature) : NULL;
    new_method->return_signature = return_signature ? strdup(return_signature) : NULL;
    new_method->handler = handler;
    new_method->user_data = user_data;
    
    /* Add to list HEAD */
    new_method->next = iface->methods;
    iface->methods = new_method;
    
    return new_method;
}

/* Find method in module */
DBUS_METHOD *module_find_method(DBUS_MODULE *module, const char *interface_name, const char *method_name, const char *object_path) {
    if (!module || !interface_name || !method_name || !object_path) {
        return NULL;
    }
    
    DBUS_INTERFACE *iface = module->interfaces;
    while (iface) {
        /* Check interface name and object path */
        if (strcmp(iface->name, interface_name) == 0 &&
            iface->object_path && strcmp(iface->object_path, object_path) == 0) {
            
            DBUS_METHOD *method = iface->methods;
            while (method) {
                if (strcmp(method->name, method_name) == 0) {
                    return method;
                }
                method = method->next;
            }
        }
        iface = iface->next;
    }
    
    return NULL;
}