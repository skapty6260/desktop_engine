#ifndef DBUS_BUFFER_MODULE_H
#define DBUS_BUFFER_MODULE_H

#include <dbus-server/module-lib.h>
#include <dbus/dbus.h>

DBusHandlerResult test_handler(DBusConnection *conn,
                                      DBusMessage *msg,
                                      void *user_data);

/* Создание тестового модуля */
DBUS_MODULE *create_test_module(void);

#endif