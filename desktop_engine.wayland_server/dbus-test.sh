#!/bin/bash

# Скрипт для тестирования D-Bus модуля

echo "Building project..."
meson setup build
meson compile -C build

echo -e "\nStarting server in background..."
./build/desktop_engine_wayland_server &
SERVER_PID=$!

echo "Server started with PID: $SERVER_PID"

# Запускаем dbus-monitor в фоне для мониторинга трафика
echo -e "\nStarting D-Bus monitor in background..."
DBUS_MONITOR_PID=""
DBUS_MONITOR_LOG="/tmp/dbus-monitor-$$.log"

# Запускаем dbus-monitor с фильтром для нашего сервиса
dbus-monitor --session "type='method_call',interface='org.skapty6260.DesktopEngine.TestModule'" \
             "type='method_return',sender='org.skapty6260.DesktopEngine'" \
             "type='error',sender='org.skapty6260.DesktopEngine'" \
             "type='signal',interface='org.skapty6260.DesktopEngine.TestModule'" \
              > "$DBUS_MONITOR_LOG" 2>&1 &
DBUS_MONITOR_PID=$!

echo "D-Bus monitor started with PID: $DBUS_MONITOR_PID (log: $DBUS_MONITOR_LOG)"

# Ждем запуска сервера и монитора
sleep 3

echo -e "\n=== Current D-Bus services ==="
dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -E "(DesktopEngine|skapt)"

echo -e "\nTesting D-Bus service..."

# Тест 1: Проверка доступности сервиса
echo "Test 1: Checking service availability..."
dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "org.skapty6260.DesktopEngine"
if [ $? -eq 0 ]; then
    echo "✓ Service is available"
    SERVICE_NAME="org.skapty6260.DesktopEngine"
else
    # Проверяем альтернативное имя (с 'g' вместо 'y')
    echo "Checking alternative service name (org.skaptg6260.DesktopEngine)..."
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "org.skaptg6260.DesktopEngine"
    if [ $? -eq 0 ]; then
        echo "✓ Service is available (with 'g')"
        SERVICE_NAME="org.skaptg6260.DesktopEngine"
    else
        echo "✗ Service not found"
        echo "Showing all services containing 'skapt':"
        dbus-send --session --dest=org.freedesktop.DBus \
            --type=method_call --print-reply \
            /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -i skapt
        kill $SERVER_PID 2>/dev/null
        kill $DBUS_MONITOR_PID 2>/dev/null
        exit 1
    fi
fi

# Функция для выполнения теста с таймаутом
run_dbus_test() {
    local description=$1
    local command=$2
    
    echo -e "\n$description"
    echo "Command: $command"
    
    # Выполняем команду с таймаутом
    timeout 10s bash -c "$command" 2>&1
    
    local exit_code=$?
    if [ $exit_code -eq 124 ]; then
        echo "✗ Timeout (10 seconds)"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "✗ Failed with exit code: $exit_code"
        return 1
    else
        echo "✓ Success"
        return 0
    fi
}

# Тест 2: Метод Ping
run_dbus_test "Test 2: Testing Ping method..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.Ping \
    string:'Test Message'"

# Тест 3: Метод GetSystemInfo
run_dbus_test "Test 3: Testing GetSystemInfo method..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.GetSystemInfo"

# Тест 4: Метод Calculate
run_dbus_test "Test 4: Testing Calculate method..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.Calculate \
    string:'add' double:10.5 double:5.2"

# Тест 5: Интроспекция интерфейса
run_dbus_test "Test 5: Interface introspection..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.freedesktop.DBus.Introspectable.Introspect"

# Тест 6: Получение свойств
run_dbus_test "Test 6: Testing properties..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.freedesktop.DBus.Properties.Get \
    string:'org.skapty6260.DesktopEngine.TestModule' \
    string:'Enabled'"

# Тест 7: Получение всех свойств
run_dbus_test "Test 7: Testing GetAll properties..." \
    "dbus-send --session --type=method_call --print-reply \
    --dest='$SERVICE_NAME' \
    /org/skapty6260/DesktopEngine/TestModule \
    org.freedesktop.DBus.Properties.GetAll \
    string:'org.skapty6260.DesktopEngine.TestModule'"

# Тест 8: Запуск тестового клиента
echo -e "\nTest 8: Running test client..."
if [ -f ./build/test-dbus-client ]; then
    ./build/test-dbus-client
else
    echo "Test client not found, building..."
    meson compile -C build test-dbus-client
    if [ -f ./build/test-dbus-client ]; then
        ./build/test-dbus-client
    else
        echo "✗ Failed to build test client"
    fi
fi

# echo -e "\n=== D-Bus Monitor Log Summary ==="
# echo "Last 20 lines from dbus-monitor:"
# tail -20 "$DBUS_MONITOR_LOG" | while IFS= read -r line; do
#     echo "  $line"
done

echo -e "\nStopping D-Bus monitor..."
kill $DBUS_MONITOR_PID 2>/dev/null
wait $DBUS_MONITOR_PID 2>/dev/null

echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# Сохраняем полный лог dbus-monitor
# echo -e "\nFull D-Bus monitor log saved to: $DBUS_MONITOR_LOG"
# echo "To view: cat $DBUS_MONITOR_LOG"

# echo -e "\nAll tests completed!"