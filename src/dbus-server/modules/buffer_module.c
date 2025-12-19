#include <dbus-server/modules/buffer_module.h>
#include <logger.h>

DBusHandlerResult test_handler(DBusConnection *conn,
                                      DBusMessage *msg,
                                      void *user_data) {
    (void)user_data; /* Не используется */
    
    LOG_INFO(LOG_MODULE_CORE, "Test method called!");
    
    /* Простой ответ */
    const char *response = "Hello from test module!";
    DBusMessage *reply = dbus_message_new_method_return(msg);
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    
    return DBUS_HANDLER_RESULT_HANDLED;
}

/* Создание тестового модуля */
DBUS_MODULE *create_test_module(void) {
    /* Создаем модуль */
    DBUS_MODULE *module = module_create("TestModule");
    if (!module) return NULL;
    
    /* Добавляем один интерфейс */
    DBUS_INTERFACE *iface = module_add_interface(
        module,
        "org.skapty6260.DesktopEngine.Test",
        "/org/skapty6260/DesktopEngine/Test"
    );

    iface->object_path = strdup("/org/skapty6260/DesktopEngine/Test");
    
    if (!iface) {
        module_destroy(module);
        return NULL;
    }
    
    /* Добавляем один метод */
    interface_add_method(iface,
        "Hello",        /* Имя метода */
        "",             /* Нет входных аргументов */
        "s",            /* Возвращает строку */
        test_handler,   /* Обработчик */
        NULL            /* Нет пользовательских данных */
    );
    
    LOG_INFO(LOG_MODULE_CORE, "Test module created with 1 interface, 1 method");
    
    return module;
}