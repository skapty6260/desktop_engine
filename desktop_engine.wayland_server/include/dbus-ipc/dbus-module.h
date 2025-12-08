#ifndef DBUS_MODULE_H
#define DBUS_MODULE_H

#include <dbus-ipc/dbus-core.h>

bool dbus_module_register(struct dbus_server *server,
                                  const char *name,
                                  const char *interface_name,
                                  const char *object_path,
                                  dbus_message_handler_t handler,
                                  void *user_data);

void dbus_module_unregister(struct dbus_server *server, const char *name);

#endif