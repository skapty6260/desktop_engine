#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Для sleep()
#include <dbus/dbus.h>

#define BUS_NAME "org.skapty6260.DesktopEngine"
#define OBJECT_PATH "/org/skapty6260/DesktopEngine/TestModule"
#define INTERFACE "org.skapty6260.DesktopEngine.TestModule"

// Функция для проверки доступности сервиса
static int wait_for_service(DBusConnection *conn, const char *service_name, int max_retries) {
    DBusError err;
    
    for (int i = 0; i < max_retries; i++) {
        dbus_error_init(&err);
        
        // Запрашиваем список сервисов
        DBusMessage *msg = dbus_message_new_method_call(
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "ListNames");
        
        if (!msg) {
            printf("Failed to create ListNames message\n");
            return 0;
        }
        
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(
            conn, msg, 1000, &err);
        
        dbus_message_unref(msg);
        
        if (dbus_error_is_set(&err)) {
            printf("Retry %d/%d: Cannot check service list: %s\n", 
                   i + 1, max_retries, err.message);
            dbus_error_free(&err);
        } else if (reply) {
            DBusMessageIter iter;
            dbus_message_iter_init(reply, &iter);
            
            if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
                DBusMessageIter array_iter;
                dbus_message_iter_recurse(&iter, &array_iter);
                
                while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
                    const char *name;
                    dbus_message_iter_get_basic(&array_iter, &name);
                    
                    if (strcmp(name, service_name) == 0) {
                        printf("Service %s is available (attempt %d/%d)\n", 
                               service_name, i + 1, max_retries);
                        dbus_message_unref(reply);
                        return 1;
                    }
                    dbus_message_iter_next(&array_iter);
                }
            }
            dbus_message_unref(reply);
            printf("Retry %d/%d: Service %s not found yet\n", 
                   i + 1, max_retries, service_name);
        }
        
        // Ждем перед следующей попыткой
        sleep(5000);
    }
    
    return 0;
}

// Функция для тестирования с ретраями
static int test_with_retry(DBusConnection *conn, 
                          const char *method_name,
                          DBusMessage *(*create_message)(void),
                          void (*process_reply)(DBusMessage *)) {
    DBusError err;
    int retries = 3;
    
    for (int attempt = 1; attempt <= retries; attempt++) {
        dbus_error_init(&err);
        
        printf("Attempt %d/%d: %s... ", attempt, retries, method_name);
        fflush(stdout);
        
        // Создаем сообщение
        DBusMessage *msg = create_message();
        if (!msg) {
            printf("FAILED to create message\n");
            return 0;
        }
        
        // Отправляем с таймаутом 3 секунды
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(
            conn, msg, 3000, &err);
        
        dbus_message_unref(msg);
        
        if (dbus_error_is_set(&err)) {
            printf("ERROR: %s", err.message);
            if (attempt < retries) {
                printf(" (retrying...)\n");
            } else {
                printf("\n");
            }
            dbus_error_free(&err);
            
            if (attempt < retries) {
                sleep(3000);
                continue;
            }
            return 0;
        }
        
        if (!reply) {
            printf("ERROR: No reply received");
            if (attempt < retries) {
                printf(" (retrying...)\n");
                sleep(3000);
                continue;
            } else {
                printf("\n");
                return 0;
            }
        }
        
        printf("SUCCESS\n");
        
        // Обрабатываем ответ
        if (process_reply) {
            process_reply(reply);
        }
        
        dbus_message_unref(reply);
        return 1;
    }
    
    return 0;
}

// Обертки для создания сообщений
static DBusMessage *create_ping_message(void) {
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "Ping");
    
    if (msg) {
        const char *test_msg = "Hello from test client!";
        dbus_message_append_args(msg,
                                DBUS_TYPE_STRING, &test_msg,
                                DBUS_TYPE_INVALID);
    }
    return msg;
}

static DBusMessage *create_system_info_message(void) {
    return dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "GetSystemInfo");
}

static DBusMessage *create_calculate_message(void) {
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "Calculate");
    
    if (msg) {
        const char *operation = "add";
        double a = 5.5, b = 3.2;
        dbus_message_append_args(msg,
                                DBUS_TYPE_STRING, &operation,
                                DBUS_TYPE_DOUBLE, &a,
                                DBUS_TYPE_DOUBLE, &b,
                                DBUS_TYPE_INVALID);
    }
    return msg;
}

static DBusMessage *create_stats_message(void) {
    return dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "GetStats");
}

// Обработчики ответов
static void process_ping_reply(DBusMessage *reply) {
    DBusError err;
    dbus_error_init(&err);
    
    const char *response;
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_STRING, &response,
                              DBUS_TYPE_INVALID)) {
        printf("    Response: %s\n", response);
    } else {
        printf("    Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
}

static void process_system_info_reply(DBusMessage *reply) {
    DBusError err;
    dbus_error_init(&err);
    
    const char *name, *version;
    dbus_uint64_t uptime;
    dbus_uint32_t pid;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &version,
                              DBUS_TYPE_UINT64, &uptime,
                              DBUS_TYPE_UINT32, &pid,
                              DBUS_TYPE_INVALID)) {
        printf("    System Info:\n");
        printf("      Name: %s\n", name);
        printf("      Version: %s\n", version);
        printf("      Uptime: %lu µs\n", (unsigned long)uptime);
        printf("      PID: %u\n", pid);
    } else {
        printf("    Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
}

static void process_calculate_reply(DBusMessage *reply) {
    DBusError err;
    dbus_error_init(&err);
    
    double result;
    const char *error_msg;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_DOUBLE, &result,
                              DBUS_TYPE_STRING, &error_msg,
                              DBUS_TYPE_INVALID)) {
        if (strlen(error_msg) > 0) {
            printf("    Error: %s\n", error_msg);
        } else {
            printf("    Result: 5.5 + 3.2 = %.2f\n", result);
        }
    } else {
        printf("    Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
}

static void process_stats_reply(DBusMessage *reply) {
    DBusError err;
    dbus_error_init(&err);
    
    dbus_uint32_t call_count;
    dbus_uint64_t last_called;
    double avg_time;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_UINT32, &call_count,
                              DBUS_TYPE_UINT64, &last_called,
                              DBUS_TYPE_DOUBLE, &avg_time,
                              DBUS_TYPE_INVALID)) {
        printf("    Stats:\n");
        printf("      Call count: %u\n", call_count);
        printf("      Last called: %lu µs ago\n", (unsigned long)last_called);
        printf("      Average time: %.2f ms\n", avg_time * 1000);
    } else {
        printf("    Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
}

int main(void) {
    DBusError err;
    DBusConnection *conn;
    
    dbus_error_init(&err);
    
    printf("=== D-Bus Test Client (with retries) ===\n\n");
    
    // Подключаемся к session bus
    printf("Connecting to D-Bus session bus...\n");
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        printf("FAILED to connect to D-Bus: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    
    printf("Connected to D-Bus\n");
    
    // Устанавливаем более агрессивный таймаут для всех операций
    // Это поможет быстрее обнаруживать проблемы
    // dbus_connection_set_timeout(conn, 5000);
    
    // Ждем сервис (максимум 10 секунд)
    printf("\nWaiting for service '%s'...\n", BUS_NAME);
    if (!wait_for_service(conn, BUS_NAME, 20)) {  // 20 попыток * 500ms = 10 секунд
        printf("ERROR: Service '%s' not available after waiting\n", BUS_NAME);
        
        // Выводим список доступных сервисов для отладки
        printf("\nAvailable services:\n");
        dbus_error_init(&err);
        DBusMessage *msg = dbus_message_new_method_call(
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "ListNames");
        
        if (msg) {
            DBusMessage *reply = dbus_connection_send_with_reply_and_block(
                conn, msg, 2000, &err);
            dbus_message_unref(msg);
            
            if (!dbus_error_is_set(&err) && reply) {
                DBusMessageIter iter;
                dbus_message_iter_init(reply, &iter);
                
                if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
                    DBusMessageIter array_iter;
                    dbus_message_iter_recurse(&iter, &array_iter);
                    
                    int count = 0;
                    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
                        const char *name;
                        dbus_message_iter_get_basic(&array_iter, &name);
                        printf("  - %s\n", name);
                        count++;
                        dbus_message_iter_next(&array_iter);
                    }
                    printf("Total: %d services\n", count);
                }
                dbus_message_unref(reply);
            } else {
                printf("  Could not get service list: %s\n", err.message);
                dbus_error_free(&err);
            }
        }
        
        dbus_connection_unref(conn);
        return 1;
    }
    
    printf("\n=== Starting tests ===\n\n");
    
    // Тестируем методы с ретраями
    int success_count = 0;
    int total_tests = 4;
    
    printf("1. Testing Ping method:\n");
    if (test_with_retry(conn, "Ping", create_ping_message, process_ping_reply)) {
        success_count++;
    }
    printf("\n");
    
    printf("2. Testing GetSystemInfo method:\n");
    if (test_with_retry(conn, "GetSystemInfo", create_system_info_message, process_system_info_reply)) {
        success_count++;
    }
    printf("\n");
    
    printf("3. Testing Calculate method:\n");
    if (test_with_retry(conn, "Calculate", create_calculate_message, process_calculate_reply)) {
        success_count++;
    }
    printf("\n");
    
    printf("4. Testing GetStats method:\n");
    if (test_with_retry(conn, "GetStats", create_stats_message, process_stats_reply)) {
        success_count++;
    }
    printf("\n");
    
    // Итог
    printf("=== Test Results ===\n");
    printf("Passed: %d/%d tests\n", success_count, total_tests);
    
    if (success_count == total_tests) {
        printf("SUCCESS: All tests passed!\n");
    } else if (success_count > 0) {
        printf("PARTIAL SUCCESS: Some tests passed\n");
    } else {
        printf("FAILURE: No tests passed\n");
    }
    
    // Закрываем соединение
    dbus_connection_unref(conn);
    
    return (success_count == total_tests) ? 0 : 1;
}