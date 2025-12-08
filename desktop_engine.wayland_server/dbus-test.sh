#!/bin/bash

# Скрипт для тестирования D-Bus модуля

echo "Building project..."
meson setup build
meson compile -C build

echo -e "\nStarting server in background..."
./build/desktop_engine_wayland_server &
SERVER_PID=$!

echo "Server started with PID: $SERVER_PID"

# Ждем запуска сервера
sleep 2

echo -e "\nTesting D-Bus service..."

# Тест 1: Проверка доступности сервиса
echo "Test 1: Checking service availability..."
dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "org.skapty6260.DesktopEngine"
if [ $? -eq 0 ]; then
    echo "✓ Service is available"
else
    echo "✗ Service not found"
fi

# Тест 2: Метод Ping
echo -e "\nTest 2: Testing Ping method..."
dbus-send --session --type=method_call --print-reply \
    --dest=org.skapty6260.DesktopEngine \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.Ping \
    string:"Test Message"

# Тест 3: Метод GetSystemInfo
echo -e "\nTest 3: Testing GetSystemInfo method..."
dbus-send --session --type=method_call --print-reply \
    --dest=org.skapty6260.DesktopEngine \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.GetSystemInfo

# Тест 4: Метод Calculate
echo -e "\nTest 4: Testing Calculate method..."
dbus-send --session --type=method_call --print-reply \
    --dest=org.skapty6260.DesktopEngine \
    /org/skapty6260/DesktopEngine/TestModule \
    org.skapty6260.DesktopEngine.TestModule.Calculate \
    string:"add" double:10.5 double:5.2

# Тест 5: Интроспекция интерфейса
echo -e "\nTest 5: Interface introspection..."
dbus-send --session --type=method_call --print-reply \
    --dest=org.skapty6260.DesktopEngine \
    /org/skapty6260/DesktopEngine/TestModule \
    org.freedesktop.DBus.Introspectable.Introspect

# Тест 6: Запуск тестового клиента
echo -e "\nTest 6: Running test client..."
if [ -f ./build/test-dbus-client ]; then
    ./build/test-dbus-client
else
    echo "Test client not found, building..."
    meson compile -C build test-dbus-client
    ./build/test-dbus-client
fi

echo -e "\nStopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo -e "\nAll tests completed!"