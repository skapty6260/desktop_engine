#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// Обработчик сообщений (не может быть NULL)
static DBusHandlerResult message_handler(DBusConnection *connection,
                                         DBusMessage *message,
                                         void *user_data) {
    
    printf("Message handler called\n");
    
    if (dbus_message_is_signal(message, 
        "org.skapty6260.DesktopEngine.Buffer", 
        "Updated")) {
        
        DBusError err;
        dbus_error_init(&err);
        
        char *info = NULL;
        if (dbus_message_get_args(message, &err, 
                                 DBUS_TYPE_STRING, &info,
                                 DBUS_TYPE_INVALID)) {
            printf("Buffer Updated: %s\n", info);
        } else {
            fprintf(stderr, "Failed to parse signal: %s\n", err.message);
            dbus_error_free(&err);
        }
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    // Для всех остальных сообщений
    printf("Received other message: interface=%s, member=%s\n",
           dbus_message_get_interface(message) ?: "(none)",
           dbus_message_get_member(message) ?: "(none)");
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main() {
    DBusError err;
    DBusConnection *conn;
    const char *unique_name;
    
    dbus_error_init(&err);
    
    // Подключаемся к сессионной шине
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    
    // Получаем уникальное имя подключения
    unique_name = dbus_bus_get_unique_name(conn);
    if (unique_name) {
        printf("Connected to DBus with unique name: %s\n", unique_name);
    }
    
    // Запрашиваем имя для нашего клиента
    int ret = dbus_bus_request_name(conn, 
                                    "org.skapty6260.DesktopEngine.BufferMonitor",
                                    DBUS_NAME_FLAG_REPLACE_EXISTING,
                                    &err);
    
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    
    if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        printf("Successfully acquired bus name\n");
    } else {
        printf("Bus name request result: %d\n", ret);
    }
    
    // Добавляем фильтр для сигналов
    dbus_bus_add_match(conn, 
        "type='signal',"
        "interface='org.skapty6260.DesktopEngine.Buffer',"
        "path='/org/skapty6260/DesktopEngine/Buffer',"
        "sender='org.skapty6260.DesktopEngine'",
        &err);
    
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Match Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    
    printf("Match rule added successfully\n");
    
    // Список всех правил для проверки (убрана, чтобы не вызывала ошибку)
    // ...
    
    printf("\nMonitoring buffer updates...\n");
    
    // Проверяем конкретный сервис (убрана, чтобы не вызывала ошибку)
    // ...
    
    // ДОБАВЛЯЕМ ФИЛЬТР С ФУНКЦИЕЙ-ОБРАБОТЧИКОМ
    if (!dbus_connection_add_filter(conn, message_handler, NULL, NULL)) {
        fprintf(stderr, "Failed to add filter\n");
        return 1;
    }
    
    // Основной цикл с правильной обработкой
    printf("Entering main loop...\n");
    
    while (1) {
        // Блокирующая обработка сообщений
        dbus_connection_read_write_dispatch(conn, 1000); // 1 секунда таймаут
        
        // Также можно использовать неблокирующий вариант:
        // dbus_connection_read_write(conn, 100); // 100ms
        // DBusMessage *msg = dbus_connection_pop_message(conn);
        // while (msg) {
        //     // Обработка сообщений вручную
        //     dbus_message_unref(msg);
        //     msg = dbus_connection_pop_message(conn);
        // }
        
        // usleep(100000); // 100ms - если используете неблокирующий вариант
    }
    
    // Очистка (никогда не выполнится, но для правильности)
    dbus_connection_remove_filter(conn, message_handler, NULL);
    dbus_connection_unref(conn);
    
    return 0;
}