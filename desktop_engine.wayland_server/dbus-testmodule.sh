#!/bin/bash
# test-module-methods.sh - Проверка методов test module

echo "=== Test Module Methods Test ==="
echo "Starting server with --no-async --log-level debug..."

# Запускаем сервер
./build/desktop_engine_wayland_server --no-async --log-level debug &
SERVER_PID=$!
echo "Server started with PID: $SERVER_PID"

# Запускаем dbus-monitor для мониторинга
DBUS_MONITOR_LOG="/tmp/dbus-monitor-methods-$$.log"
dbus-monitor --session "type='method_call',interface='org.skapty6260.DesktopEngine.TestModule'" \
             "type='method_return',sender='org.skapty6260.DesktopEngine'" \
             > "$DBUS_MONITOR_LOG" 2>&1 &
MONITOR_PID=$!

# Ждем запуска
sleep 3

# Конфигурация
SERVICE_NAME="org.skapty6260.DesktopEngine"
OBJECT_PATH="/org/skapty6260/DesktopEngine/TestModule"
INTERFACE="org.skapty6260.DesktopEngine.TestModule"

echo -e "\n=== Step 1: Verify service ==="
if ! dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
    echo "✗ Service not found"
    kill $SERVER_PID $MONITOR_PID 2>/dev/null
    exit 1
fi
echo "✓ Service verified"

# Функция для тестирования методов
test_method() {
    local name=$1
    local command=$2
    
    echo -e "\n--- Testing $name ---"
    echo "Command: $command"
    
    timeout 5s bash -c "$command" 2>&1
    local result=$?
    
    if [ $result -eq 0 ]; then
        echo "✓ $name: SUCCESS"
        return 0
    elif [ $result -eq 124 ]; then
        echo "✗ $name: TIMEOUT (5 seconds)"
        return 1
    else
        echo "✗ $name: FAILED (exit code: $result)"
        return 1
    fi
}

echo -e "\n=== Step 2: Test Module Methods ==="

# 1. Introspection
test_method "Introspection" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Introspectable.Introspect"

# 2. Ping method
test_method "Ping" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Ping' string:'Hello from test script'"

# 3. GetSystemInfo method
test_method "GetSystemInfo" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.GetSystemInfo'"

# 4. Calculate method (add)
test_method "Calculate (add)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Calculate' string:'add' double:15.7 double:3.2"

# 5. Calculate method (multiply)
test_method "Calculate (multiply)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Calculate' string:'multiply' double:4.5 double:2.0"

# 6. GetStats method
test_method "GetStats" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.GetStats'"

# 7. Properties: Get Enabled
test_method "Property Get (Enabled)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Properties.Get \
    string:'$INTERFACE' string:'Enabled'"

# 8. Properties: GetAll
test_method "Property GetAll" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Properties.GetAll \
    string:'$INTERFACE'"

echo -e "\n=== Step 3: Monitor Log Summary ==="
echo "Last 15 lines from dbus-monitor:"
tail -15 "$DBUS_MONITOR_LOG" 2>/dev/null | while IFS= read -r line; do
    echo "  $line"
done

echo -e "\n=== Step 4: Cleanup ==="
echo "Stopping monitor (PID: $MONITOR_PID)..."
kill $MONITOR_PID 2>/dev/null 2>/dev/null

echo "Stopping server (PID: $SERVER_PID)..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "\n=== Test Module Methods Test Complete ==="
echo "Log saved to: $DBUS_MONITOR_LOG"