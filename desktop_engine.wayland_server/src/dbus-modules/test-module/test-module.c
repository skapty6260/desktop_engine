#include "test-module.h"
#include <dbus-ipc/dbus-module.h>
#include <logger.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>        // Для clock_gettime, timespec
#include <unistd.h>      // Для getpid()
#include <sys/time.h>    // Для gettimeofday()
#include <math.h>        // Для pow()
#include <stdbool.h>     // Для bool типа

// Данные модуля (синглтон)
static test_module_data_t *module_data = NULL;

// XML описание интерфейса (можно загружать из файла)
static const char *test_module_xml = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "        \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "    <interface name=\"org.skapty6260.DesktopEngine.TestModule\">\n"
    "        <method name=\"Ping\">\n"
    "            <arg name=\"message\" type=\"s\" direction=\"in\"/>\n"
    "            <arg name=\"reply\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <method name=\"GetSystemInfo\">\n"
    "            <arg name=\"name\" type=\"s\" direction=\"out\"/>\n"
    "            <arg name=\"version\" type=\"s\" direction=\"out\"/>\n"
    "            <arg name=\"uptime\" type=\"t\" direction=\"out\"/>\n"
    "            <arg name=\"pid\" type=\"u\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <method name=\"Calculate\">\n"
    "            <arg name=\"operation\" type=\"s\" direction=\"in\"/>\n"
    "            <arg name=\"a\" type=\"d\" direction=\"in\"/>\n"
    "            <arg name=\"b\" type=\"d\" direction=\"in\"/>\n"
    "            <arg name=\"result\" type=\"d\" direction=\"out\"/>\n"
    "            <arg name=\"error_message\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <method name=\"GetStats\">\n"
    "            <arg name=\"call_count\" type=\"u\" direction=\"out\"/>\n"
    "            <arg name=\"last_called\" type=\"t\" direction=\"out\"/>\n"
    "            <arg name=\"average_time\" type=\"d\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <signal name=\"TestEvent\">\n"
    "            <arg name=\"event_type\" type=\"s\"/>\n"
    "            <arg name=\"data\" type=\"v\"/>\n"
    "            <arg name=\"timestamp\" type=\"t\"/>\n"
    "        </signal>\n"
    "        <property name=\"Enabled\" type=\"b\" access=\"readwrite\"/>\n"
    "        <property name=\"Version\" type=\"s\" access=\"read\"/>\n"
    "        <property name=\"CallCount\" type=\"u\" access=\"read\"/>\n"
    "    </interface>\n"
    "</node>";

// Вспомогательная функция для получения текущего времени в микросекундах
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Обработчик метода Ping
static DBusHandlerResult handle_ping(DBusConnection *connection,
                                    DBusMessage *message,
                                    test_module_data_t *data) {
    DBusError err;
    dbus_error_init(&err);

    DBUS_DEBUG("test-module: Ping handler called");
    
    const char *input_message;
    if (!dbus_message_get_args(message, &err,
                               DBUS_TYPE_STRING, &input_message,
                               DBUS_TYPE_INVALID)) {
        DBUS_ERROR("Failed to parse Ping arguments: %s", err.message);
        dbus_error_free(&err);
        
        DBusMessage *reply = dbus_message_new_error(message,
            DBUS_ERROR_INVALID_ARGS, "Expected a string argument");
        if (reply) {
            // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Добавляем flush после отправки
            dbus_connection_send(connection, reply, NULL);
            dbus_connection_flush(connection);  // <-- ДОБАВИТЬ ЭТО
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    data->call_count++;
    DBUS_DEBUG("TestModule.Ping called with: %s (call #%u)", input_message, data->call_count);
    
    // Создаем ответ
    char reply_msg[256];
    snprintf(reply_msg, sizeof(reply_msg), 
             "Pong! Received: '%s' (call #%u)", 
             input_message, data->call_count);
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply,
                            DBUS_TYPE_STRING, &reply_msg,
                            DBUS_TYPE_INVALID);
    
                            if (!dbus_connection_get_is_connected(connection)) {
        DBUS_ERROR("D-Bus connection is not connected, cannot send reply");
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    
    // Отправляем с серийным номером
    dbus_uint32_t serial = dbus_message_get_serial(message);
    if (serial) {
        dbus_message_set_reply_serial(reply, serial);
    }

    if (!dbus_connection_send(connection, reply, NULL)) {
        DBUS_ERROR("Failed to send reply");
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    dbus_connection_flush(connection);  // <-- ЭТО САМОЕ ВАЖНОЕ
    dbus_message_unref(reply);
    
    DBUS_DEBUG("Ping reply sent and flushed");
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Обработчик метода GetSystemInfo
static DBusHandlerResult handle_get_system_info(DBusConnection *connection,
                                               DBusMessage *message,
                                               test_module_data_t *data) {
    data->call_count++;
    
    const char *name = "DesktopEngine Test Module";
    const char *version = "1.0.0";
    uint64_t uptime = get_time_us(); // Время с запуска модуля
    uint32_t pid = getpid();
    
    DBUS_DEBUG("TestModule.GetSystemInfo called");
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply,
                            DBUS_TYPE_STRING, &name,
                            DBUS_TYPE_STRING, &version,
                            DBUS_TYPE_UINT64, &uptime,
                            DBUS_TYPE_UINT32, &pid,
                            DBUS_TYPE_INVALID);
    
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Обработчик метода Calculate
static DBusHandlerResult handle_calculate(DBusConnection *connection,
                                         DBusMessage *message,
                                         test_module_data_t *data) {
    DBusError err;
    dbus_error_init(&err);
    
    const char *operation;
    double a, b;
    
    if (!dbus_message_get_args(message, &err,
                               DBUS_TYPE_STRING, &operation,
                               DBUS_TYPE_DOUBLE, &a,
                               DBUS_TYPE_DOUBLE, &b,
                               DBUS_TYPE_INVALID)) {
        DBUS_ERROR("Failed to parse Calculate arguments: %s", err.message);
        dbus_error_free(&err);
        
        DBusMessage *reply = dbus_message_new_error(message,
            DBUS_ERROR_INVALID_ARGS, "Expected (string, double, double)");
        if (reply) {
            dbus_connection_send(connection, reply, NULL);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    data->call_count++;
    
    double result = 0.0;
    const char *error_message = "";
    
    DBUS_DEBUG("TestModule.Calculate called: %s %f %f", operation, a, b);
    
    if (strcmp(operation, "add") == 0) {
        result = a + b;
    } else if (strcmp(operation, "subtract") == 0) {
        result = a - b;
    } else if (strcmp(operation, "multiply") == 0) {
        result = a * b;
    } else if (strcmp(operation, "divide") == 0) {
        if (b != 0.0) {
            result = a / b;
        } else {
            error_message = "Division by zero";
        }
    } else if (strcmp(operation, "power") == 0) {
        result = pow(a, b);
    } else {
        error_message = "Unknown operation";
    }
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply,
                            DBUS_TYPE_DOUBLE, &result,
                            DBUS_TYPE_STRING, &error_message,
                            DBUS_TYPE_INVALID);
    
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Обработчик метода GetStats
static DBusHandlerResult handle_get_stats(DBusConnection *connection,
                                         DBusMessage *message,
                                         test_module_data_t *data) {
    data->call_count++;
    
    uint32_t call_count = data->call_count;
    uint64_t last_called = get_time_us();
    double average_time = 0.1; // Примерное значение
    
    DBUS_DEBUG("TestModule.GetStats called");
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    dbus_message_append_args(reply,
                            DBUS_TYPE_UINT32, &call_count,
                            DBUS_TYPE_UINT64, &last_called,
                            DBUS_TYPE_DOUBLE, &average_time,
                            DBUS_TYPE_INVALID);
    
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Обработчик метода GetList
static DBusHandlerResult handle_get_list(DBusConnection *connection,
                                        DBusMessage *message,
                                        test_module_data_t *data) {
    data->call_count++;
    
    DBUS_DEBUG("TestModule.GetList called");
    
    // Создаем массив строк
    const char *items[] = {
        "Item 1",
        "Item 2",
        "Item 3",
        "Test Item",
        "Another Item"
    };
    int item_count = 5;
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    DBusMessageIter iter, array_iter;
    dbus_message_iter_init_append(reply, &iter);
    
    // Создаем массив строк
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
                                     DBUS_TYPE_STRING_AS_STRING,
                                     &array_iter);
    
    for (int i = 0; i < item_count; i++) {
        const char *item = items[i];
        dbus_message_iter_append_basic(&array_iter,
                                       DBUS_TYPE_STRING,
                                       &item);
    }
    
    dbus_message_iter_close_container(&iter, &array_iter);
    
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Обработчик метода GetConfig
static DBusHandlerResult handle_get_config(DBusConnection *connection,
                                          DBusMessage *message,
                                          test_module_data_t *data) {
    data->call_count++;
    
    DBUS_DEBUG("TestModule.GetConfig called");
    
    DBusMessage *reply = dbus_message_new_method_return(message);
    if (!reply) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    
    DBusMessageIter iter, dict_iter, variant_iter;
    dbus_message_iter_init_append(reply, &iter);
    
    // Создаем словарь a{sv}
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
                                     DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                     DBUS_TYPE_STRING_AS_STRING
                                     DBUS_TYPE_VARIANT_AS_STRING
                                     DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                     &dict_iter);
    
    // Добавляем ключ "enabled"
    const char *key1 = "enabled";
    dbus_bool_t enabled = data->enabled ? TRUE : FALSE;
    
    DBusMessageIter dict_entry_iter;
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                     NULL, &dict_entry_iter);
    
    dbus_message_iter_append_basic(&dict_entry_iter,
                                   DBUS_TYPE_STRING,
                                   &key1);
    
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_BOOLEAN_AS_STRING,
                                     &variant_iter);
    dbus_message_iter_append_basic(&variant_iter,
                                   DBUS_TYPE_BOOLEAN,
                                   &enabled);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    
    // Добавляем ключ "version"
    const char *key2 = "version";
    const char *version = data->version;
    
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                     NULL, &dict_entry_iter);
    
    dbus_message_iter_append_basic(&dict_entry_iter,
                                   DBUS_TYPE_STRING,
                                   &key2);
    
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_STRING_AS_STRING,
                                     &variant_iter);
    dbus_message_iter_append_basic(&variant_iter,
                                   DBUS_TYPE_STRING,
                                   &version);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    
    // Добавляем ключ "call_count"
    const char *key3 = "call_count";
    uint32_t call_count = data->call_count;
    
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                     NULL, &dict_entry_iter);
    
    dbus_message_iter_append_basic(&dict_entry_iter,
                                   DBUS_TYPE_STRING,
                                   &key3);
    
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_UINT32_AS_STRING,
                                     &variant_iter);
    dbus_message_iter_append_basic(&variant_iter,
                                   DBUS_TYPE_UINT32,
                                   &call_count);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    
    // Добавляем ключ "temperature"
    const char *key4 = "temperature";
    double temperature = data->temperature;
    
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                     NULL, &dict_entry_iter);
    
    dbus_message_iter_append_basic(&dict_entry_iter,
                                   DBUS_TYPE_STRING,
                                   &key4);
    
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_DOUBLE_AS_STRING,
                                     &variant_iter);
    dbus_message_iter_append_basic(&variant_iter,
                                   DBUS_TYPE_DOUBLE,
                                   &temperature);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    
    dbus_message_iter_close_container(&iter, &dict_iter);
    
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Главный обработчик сообщений модуля
DBusHandlerResult test_module_message_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data) {
    test_module_data_t *data = (test_module_data_t *)user_data;
    const char *method = dbus_message_get_member(message);
    
    if (!method) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    
    DBUS_DEBUG("TestModule received method: %s", method);
    
    if (strcmp(method, "Ping") == 0) {
        return handle_ping(connection, message, data);
    } else if (strcmp(method, "GetSystemInfo") == 0) {
        return handle_get_system_info(connection, message, data);
    } else if (strcmp(method, "Calculate") == 0) {
        return handle_calculate(connection, message, data);
    } else if (strcmp(method, "GetStats") == 0) {
        return handle_get_stats(connection, message, data);
    } else if (strcmp(method, "GetList") == 0) {
        return handle_get_list(connection, message, data);
    } else if (strcmp(method, "GetConfig") == 0) {
        return handle_get_config(connection, message, data);
    }
    
    // Обработка свойств (properties)
    if (strcmp(method, "Get") == 0) {
        // Обработка чтения свойств
        DBusError err;
        dbus_error_init(&err);
        
        const char *interface, *property_name;
        if (!dbus_message_get_args(message, &err,
                                   DBUS_TYPE_STRING, &interface,
                                   DBUS_TYPE_STRING, &property_name,
                                   DBUS_TYPE_INVALID)) {
            dbus_error_free(&err);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        
        DBUS_DEBUG("Property Get: %s.%s", interface, property_name);
        
        DBusMessage *reply = dbus_message_new_method_return(message);
        if (!reply) {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }
        
        DBusMessageIter iter, variant_iter;
        dbus_message_iter_init_append(reply, &iter);
        
        if (strcmp(property_name, "Enabled") == 0) {
            dbus_bool_t enabled = data->enabled ? TRUE : FALSE;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                             DBUS_TYPE_BOOLEAN_AS_STRING,
                                             &variant_iter);
            dbus_message_iter_append_basic(&variant_iter,
                                           DBUS_TYPE_BOOLEAN,
                                           &enabled);
            dbus_message_iter_close_container(&iter, &variant_iter);
        } else if (strcmp(property_name, "Version") == 0) {
            const char *version = data->version;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                             DBUS_TYPE_STRING_AS_STRING,
                                             &variant_iter);
            dbus_message_iter_append_basic(&variant_iter,
                                           DBUS_TYPE_STRING,
                                           &version);
            dbus_message_iter_close_container(&iter, &variant_iter);
        } else if (strcmp(property_name, "CallCount") == 0) {
            uint32_t call_count = data->call_count;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                             DBUS_TYPE_UINT32_AS_STRING,
                                             &variant_iter);
            dbus_message_iter_append_basic(&variant_iter,
                                           DBUS_TYPE_UINT32,
                                           &call_count);
            dbus_message_iter_close_container(&iter, &variant_iter);
        } else if (strcmp(property_name, "Temperature") == 0) {
            double temperature = data->temperature;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                             DBUS_TYPE_DOUBLE_AS_STRING,
                                             &variant_iter);
            dbus_message_iter_append_basic(&variant_iter,
                                           DBUS_TYPE_DOUBLE,
                                           &temperature);
            dbus_message_iter_close_container(&iter, &variant_iter);
        } else {
            dbus_message_unref(reply);
            DBusMessage *error = dbus_message_new_error(message,
                DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property");
            if (error) {
                dbus_connection_send(connection, error, NULL);
                dbus_message_unref(error);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    // Обработка установки свойств
    if (strcmp(method, "Set") == 0) {
        DBusError err;
        dbus_error_init(&err);
        
        const char *interface, *property_name;
        DBusMessageIter iter, variant_iter;
        
        if (!dbus_message_get_args(message, &err,
                                   DBUS_TYPE_STRING, &interface,
                                   DBUS_TYPE_STRING, &property_name,
                                   DBUS_TYPE_INVALID)) {
            dbus_error_free(&err);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        
        // Получаем значение свойства
        if (!dbus_message_iter_init(message, &iter) ||
            !dbus_message_iter_next(&iter) ||
            dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        
        dbus_message_iter_recurse(&iter, &variant_iter);
        
        if (strcmp(property_name, "Enabled") == 0) {
            if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t enabled;
                dbus_message_iter_get_basic(&variant_iter, &enabled);
                data->enabled = (enabled != FALSE);
                DBUS_DEBUG("Property Set: Enabled = %s", 
                          data->enabled ? "true" : "false");
            }
        }
        
        // Отправляем сигнал об изменении свойства
        DBusMessage *signal = dbus_message_new_signal(
            "/org/skapty6260/DesktopEngine/TestModule",
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged");
        
        if (signal) {
            const char *iface = "org.skapty6260.DesktopEngine.TestModule";
            DBusMessageIter sig_iter, dict_iter, entry_iter, var_iter;
            
            dbus_message_iter_init_append(signal, &sig_iter);
            
            // Интерфейс
            dbus_message_iter_append_basic(&sig_iter,
                                          DBUS_TYPE_STRING,
                                          &iface);
            
            // Измененные свойства (словарь)
            dbus_message_iter_open_container(&sig_iter, DBUS_TYPE_ARRAY,
                                             DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                             DBUS_TYPE_STRING_AS_STRING
                                             DBUS_TYPE_VARIANT_AS_STRING
                                             DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                             &dict_iter);
            
            const char *key = property_name;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                             NULL, &entry_iter);
            
            dbus_message_iter_append_basic(&entry_iter,
                                          DBUS_TYPE_STRING,
                                          &key);
            
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT,
                                             DBUS_TYPE_BOOLEAN_AS_STRING,
                                             &var_iter);
            
            dbus_bool_t enabled = data->enabled ? TRUE : FALSE;
            dbus_message_iter_append_basic(&var_iter,
                                          DBUS_TYPE_BOOLEAN,
                                          &enabled);
            
            dbus_message_iter_close_container(&entry_iter, &var_iter);
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
            dbus_message_iter_close_container(&sig_iter, &dict_iter);
            
            // Неизмененные свойства (пустой массив)
            const char *empty_array = "";
            dbus_message_iter_append_basic(&sig_iter,
                                          DBUS_TYPE_ARRAY,
                                          &empty_array);
            
            dbus_connection_send(connection, signal, NULL);
            dbus_message_unref(signal);
        }
        
        // Отправляем ответ
        DBusMessage *reply = dbus_message_new_method_return(message);
        if (reply) {
            dbus_connection_send(connection, reply, NULL);
            dbus_message_unref(reply);
        }
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    // Если метод неизвестен, возвращаем ошибку НЕМЕДЛЕННО
    DBUS_ERROR("Unknown method: %s", method);
    
    // Создаем сообщение об ошибке
    DBusMessage *error = dbus_message_new_error(message,
        DBUS_ERROR_UNKNOWN_METHOD,
        "Unknown method");
    
    if (error) {
        dbus_connection_send(connection, error, NULL);
        dbus_connection_flush(connection);  // Важно: флашим соединение
        dbus_message_unref(error);
    }
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

// Получение XML описания интерфейса
const char *test_module_get_xml(void) {
    return test_module_xml;
}

// Регистрация модуля
bool test_module_register(struct dbus_server *server) {
    if (!server) {
        DBUS_ERROR("Invalid server for test module registration");
        return false;
    }
    
    // Создаем данные модуля
    module_data = calloc(1, sizeof(test_module_data_t));
    if (!module_data) {
        DBUS_ERROR("Failed to allocate test module data");
        return false;
    }
    
    // Инициализируем данные
    module_data->call_count = 0;
    module_data->enabled = true;
    module_data->temperature = 23.5; // Примерная температура
    module_data->version = strdup("1.0.0");
    clock_gettime(CLOCK_MONOTONIC, &module_data->start_time);
    
    DBUS_INFO("Initializing TestModule v%s", module_data->version);
    
    // Регистрируем модуль
    bool success = dbus_module_register(
        server,
        "test-module",
        "org.skapty6260.DesktopEngine.TestModule",
        "/org/skapty6260/DesktopEngine/TestModule",
        test_module_message_handler,
        module_data
    );
    
    if (!success) {
        DBUS_ERROR("Failed to register test module");
        free(module_data->version);
        free(module_data);
        module_data = NULL;
        return false;
    }
    
    DBUS_INFO("TestModule registered successfully");
    return true;
}

// Отмена регистрации модуля
void test_module_unregister(struct dbus_server *server) {
    if (!server) return;
    
    DBUS_INFO("Unregistering TestModule");
    
    // Удаляем модуль из сервера
    dbus_module_unregister(server, "test-module");
    
    // Очищаем данные
    if (module_data) {
        free(module_data->version);
        free(module_data);
        module_data = NULL;
    }
}

// Отправка тестового события
bool test_module_send_event(struct dbus_server *server,
                           const char *event_type,
                           const char *data) {
    if (!server || !server->connection) {
        return false;
    }
    
    DBusMessage *signal = dbus_message_new_signal(
        "/org/skapty6260/DesktopEngine/TestModule",
        "org.skapty6260.DesktopEngine.TestModule",
        "TestEvent");
    
    if (!signal) {
        return false;
    }
    
    DBusMessageIter iter;
    dbus_message_iter_init_append(signal, &iter);
    
    // Тип события
    dbus_message_iter_append_basic(&iter,
                                  DBUS_TYPE_STRING,
                                  &event_type);
    
    // Данные (как вариант)
    const char *str_data = data ? data : "";
    DBusMessageIter variant_iter;
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_STRING_AS_STRING,
                                     &variant_iter);
    dbus_message_iter_append_basic(&variant_iter,
                                  DBUS_TYPE_STRING,
                                  &str_data);
    dbus_message_iter_close_container(&iter, &variant_iter);
    
    // Временная метка
    uint64_t timestamp = get_time_us();
    dbus_message_iter_append_basic(&iter,
                                  DBUS_TYPE_UINT64,
                                  &timestamp);
    
    dbus_connection_send(server->connection, signal, NULL);
    dbus_message_unref(signal);
    
    DBUS_DEBUG("TestModule event sent: %s", event_type);
    return true;
}
