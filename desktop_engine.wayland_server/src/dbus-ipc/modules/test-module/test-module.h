#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include <dbus-ipc/dbus-server.h>

// Структура данных модуля
typedef struct test_module_data {
    unsigned int call_count;
    bool enabled;
    double temperature;
    char *version;
    struct timespec start_time;
} test_module_data_t;

// Функции модуля
bool test_module_register(struct dbus_server *server);
void test_module_unregister(struct dbus_server *server);

// Вспомогательные функции
const char *test_module_get_xml(void);
DBusHandlerResult test_module_message_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data);

// Функции для работы с D-Bus из других частей кода
bool test_module_send_event(struct dbus_server *server,
                           const char *event_type,
                           const char *data);
bool test_module_update_counter(struct dbus_server *server, int delta);

#endif // TEST_MODULE_H