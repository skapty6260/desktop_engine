#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define BUS_NAME "org.skapty6260.DesktopEngine"
#define OBJECT_PATH "/org/skapty6260/DesktopEngine/TestModule"
#define INTERFACE "org.skapty6260.DesktopEngine.TestModule"

static void test_ping(DBusConnection *conn) {
    DBusError err;
    dbus_error_init(&err);
    
    printf("Testing Ping method...\n");
    
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "Ping"
    );
    
    if (!msg) {
        printf("Failed to create message\n");
        return;
    }
    
    const char *test_msg = "Hello from test client!";
    dbus_message_append_args(msg,
                            DBUS_TYPE_STRING, &test_msg,
                            DBUS_TYPE_INVALID);
    
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, 5000, &err);
    
    dbus_message_unref(msg);
    
    if (dbus_error_is_set(&err)) {
        printf("Error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    
    if (!reply) {
        printf("No reply received\n");
        return;
    }
    
    const char *response;
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_STRING, &response,
                              DBUS_TYPE_INVALID)) {
        printf("Response: %s\n", response);
    } else {
        printf("Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
    
    dbus_message_unref(reply);
}

static void test_get_system_info(DBusConnection *conn) {
    DBusError err;
    dbus_error_init(&err);
    
    printf("Testing GetSystemInfo method...\n");
    
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "GetSystemInfo"
    );
    
    if (!msg) {
        printf("Failed to create message\n");
        return;
    }
    
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, 5000, &err);
    
    dbus_message_unref(msg);
    
    if (dbus_error_is_set(&err)) {
        printf("Error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    
    if (!reply) {
        printf("No reply received\n");
        return;
    }
    
    const char *name, *version;
    dbus_uint64_t uptime;
    dbus_uint32_t pid;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &version,
                              DBUS_TYPE_UINT64, &uptime,
                              DBUS_TYPE_UINT32, &pid,
                              DBUS_TYPE_INVALID)) {
        printf("System Info:\n");
        printf("  Name: %s\n", name);
        printf("  Version: %s\n", version);
        printf("  Uptime: %lu µs\n", (unsigned long)uptime);
        printf("  PID: %u\n", pid);
    } else {
        printf("Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
    
    dbus_message_unref(reply);
}

static void test_calculate(DBusConnection *conn) {
    DBusError err;
    dbus_error_init(&err);
    
    printf("Testing Calculate method...\n");
    
    // Тест сложения
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "Calculate"
    );
    
    if (!msg) {
        printf("Failed to create message\n");
        return;
    }
    
    const char *operation = "add";
    double a = 5.5, b = 3.2;
    
    dbus_message_append_args(msg,
                            DBUS_TYPE_STRING, &operation,
                            DBUS_TYPE_DOUBLE, &a,
                            DBUS_TYPE_DOUBLE, &b,
                            DBUS_TYPE_INVALID);
    
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, 5000, &err);
    
    dbus_message_unref(msg);
    
    if (dbus_error_is_set(&err)) {
        printf("Error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    
    if (!reply) {
        printf("No reply received\n");
        return;
    }
    
    double result;
    const char *error_msg;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_DOUBLE, &result,
                              DBUS_TYPE_STRING, &error_msg,
                              DBUS_TYPE_INVALID)) {
        if (strlen(error_msg) > 0) {
            printf("Error: %s\n", error_msg);
        } else {
            printf("Result: %.2f + %.2f = %.2f\n", a, b, result);
        }
    } else {
        printf("Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
    
    dbus_message_unref(reply);
}

static void test_get_stats(DBusConnection *conn) {
    DBusError err;
    dbus_error_init(&err);
    
    printf("Testing GetStats method...\n");
    
    DBusMessage *msg = dbus_message_new_method_call(
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE,
        "GetStats"
    );
    
    if (!msg) {
        printf("Failed to create message\n");
        return;
    }
    
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, 5000, &err);
    
    dbus_message_unref(msg);
    
    if (dbus_error_is_set(&err)) {
        printf("Error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    
    if (!reply) {
        printf("No reply received\n");
        return;
    }
    
    dbus_uint32_t call_count;
    dbus_uint64_t last_called;
    double avg_time;
    
    if (dbus_message_get_args(reply, &err,
                              DBUS_TYPE_UINT32, &call_count,
                              DBUS_TYPE_UINT64, &last_called,
                              DBUS_TYPE_DOUBLE, &avg_time,
                              DBUS_TYPE_INVALID)) {
        printf("Stats:\n");
        printf("  Call count: %u\n", call_count);
        printf("  Last called: %lu µs ago\n", (unsigned long)last_called);
        printf("  Average time: %.2f ms\n", avg_time * 1000);
    } else {
        printf("Failed to parse reply: %s\n", err.message);
        dbus_error_free(&err);
    }
    
    dbus_message_unref(reply);
}

int main(void) {
    DBusError err;
    DBusConnection *conn;
    
    dbus_error_init(&err);
    
    // Подключаемся к session bus
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        printf("Failed to connect to D-Bus: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    
    printf("Connected to D-Bus\n");
    printf("Testing service: %s\n", BUS_NAME);
    
    // Запускаем тесты
    test_ping(conn);
    printf("\n");
    
    test_get_system_info(conn);
    printf("\n");
    
    test_calculate(conn);
    printf("\n");
    
    test_get_stats(conn);
    printf("\n");
    
    printf("All tests completed\n");
    
    dbus_connection_unref(conn);
    return 0;
}