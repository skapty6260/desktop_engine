#!/bin/bash
# test-module-methods.sh - Проверка методов test module

echo "=== Test Module Methods Test ==="

# Очищаем предыдущие переменные D-Bus и устанавливаем правильный адрес
unset DBUS_SESSION_BUS_ADDRESS
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u)/bus"

echo "Using D-Bus address: $DBUS_SESSION_BUS_ADDRESS"
echo "Starting server with --no-async --log-level debug..."

# Запускаем сервер
./build/desktop_engine_wayland_server --no-async --log-level debug &
SERVER_PID=$!
echo "Server started with PID: $SERVER_PID"

# Запускаем dbus-monitor для мониторинга
DBUS_MONITOR_LOG="/tmp/dbus-monitor-methods-$$.log"
echo "Starting dbus-monitor (log: $DBUS_MONITOR_LOG)"

dbus-monitor --session "type='method_call',interface='org.skapty6260.DesktopEngine.TestModule'" \
             "type='method_return',sender='org.skapty6260.DesktopEngine'" \
             "type='error',sender='org.skapty6260.DesktopEngine'" \
             > "$DBUS_MONITOR_LOG" 2>&1 &
MONITOR_PID=$!
echo "dbus-monitor started with PID: $MONITOR_PID"

# Даем больше времени для запуска
echo -e "\nWaiting for server and monitor to initialize..."
sleep 5

# Конфигурация
SERVICE_NAME="org.skapty6260.DesktopEngine"
OBJECT_PATH="/org/skapty6260/DesktopEngine/TestModule"
INTERFACE="org.skapty6260.DesktopEngine.TestModule"

echo -e "\n=== Step 1: Verify service ==="
echo "Checking for service: $SERVICE_NAME"

# Проверяем несколько раз с задержкой
SERVICE_FOUND=false
for i in {1..5}; do
    echo "Attempt $i/5..."
    if dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
        echo "✓ Service '$SERVICE_NAME' found!"
        SERVICE_FOUND=true
        break
    fi
    sleep 1
done

if [ "$SERVICE_FOUND" = false ]; then
    echo "✗ Service '$SERVICE_NAME' NOT found after 5 attempts"
    echo "Available services:"
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -E -i "(desktopengine|skapt)" || echo "  None found"
    
    echo "Stopping monitor and server..."
    kill $MONITOR_PID $SERVER_PID 2>/dev/null 2>/dev/null
    exit 1
fi

echo "✓ Service verified"

# Функция для тестирования методов с улучшенной диагностикой
test_method() {
    local name=$1
    local command=$2
    
    echo -e "\n--- Testing $name ---"
    echo "Command: $(echo "$command" | sed 's/--session/--session/g' | head -c 100)..."
    
    local start_time=$(date +%s)
    local output=$(timeout 10s bash -c "$command" 2>&1)
    local result=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ $result -eq 0 ]; then
        echo "✓ $name: SUCCESS (${duration}s)"
        # Показываем краткий результат
        echo "  Response: $(echo "$output" | grep -E '(string|double|uint32|uint64)' | head -2 | sed 's/^/    /')"
        return 0
    elif [ $result -eq 124 ]; then
        echo "✗ $name: TIMEOUT (10 seconds)"
        return 1
    else
        echo "✗ $name: FAILED (exit code: $result, ${duration}s)"
        echo "  Error: $(echo "$output" | head -3 | sed 's/^/    /')"
        return 1
    fi
}

echo -e "\n=== Step 2: Test Module Methods ==="

FAILED_TESTS=0
TOTAL_TESTS=0

# 1. Introspection
((TOTAL_TESTS++))
test_method "Introspection" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Introspectable.Introspect"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 2. Ping method
((TOTAL_TESTS++))
test_method "Ping" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Ping' string:'Hello from test script'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 3. GetSystemInfo method
((TOTAL_TESTS++))
test_method "GetSystemInfo" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.GetSystemInfo'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 4. Calculate method (add)
((TOTAL_TESTS++))
test_method "Calculate (add)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Calculate' string:'add' double:15.7 double:3.2"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 5. Calculate method (multiply)
((TOTAL_TESTS++))
test_method "Calculate (multiply)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Calculate' string:'multiply' double:4.5 double:2.0"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 6. Calculate method (divide)
((TOTAL_TESTS++))
test_method "Calculate (divide)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Calculate' string:'divide' double:10.0 double:2.5"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 7. GetStats method
((TOTAL_TESTS++))
test_method "GetStats" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.GetStats'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 8. Properties: Get Enabled
((TOTAL_TESTS++))
test_method "Property Get (Enabled)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Properties.Get \
    string:'$INTERFACE' string:'Enabled'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 9. Properties: Get Version
((TOTAL_TESTS++))
test_method "Property Get (Version)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Properties.Get \
    string:'$INTERFACE' string:'Version'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 10. Properties: GetAll
((TOTAL_TESTS++))
test_method "Property GetAll" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' org.freedesktop.DBus.Properties.GetAll \
    string:'$INTERFACE'"
[ $? -ne 0 ] && ((FAILED_TESTS++))

# 11. Method with wrong arguments (negative test)
((TOTAL_TESTS++))
echo -e "\n--- Testing Error Handling (wrong arguments) ---"
test_method "Ping with no args (should fail)" \
    "dbus-send --session --dest='$SERVICE_NAME' --type=method_call --print-reply \
    '$OBJECT_PATH' '$INTERFACE.Ping'"
# Этот тест должен провалиться, поэтому инвертируем результат
if [ $? -eq 0 ]; then
    echo "✗ Unexpected success - method should fail without arguments"
    ((FAILED_TESTS++))
else
    echo "✓ Correctly failed with wrong arguments"
    ((FAILED_TESTS--)) # Уменьшаем счетчик, так как это ожидаемая ошибка
fi

echo -e "\n=== Step 3: Test Summary ==="
echo "Tests run: $TOTAL_TESTS"
echo "Passed: $((TOTAL_TESTS - FAILED_TESTS))"
echo "Failed: $FAILED_TESTS"

echo -e "\n=== Step 4: Monitor Log Summary ==="
echo "Last 30 lines from dbus-monitor:"
if [ -f "$DBUS_MONITOR_LOG" ] && [ -s "$DBUS_MONITOR_LOG" ]; then
    echo "=== D-Bus Traffic ==="
    tail -30 "$DBUS_MONITOR_LOG" | while IFS= read -r line; do
        echo "  $line"
    done
    echo "=== End of log ==="
    
    # Подсчитываем события в логе
    METHOD_CALLS=$(grep -c "method call" "$DBUS_MONITOR_LOG" 2>/dev/null || echo 0)
    METHOD_RETURNS=$(grep -c "method return" "$DBUS_MONITOR_LOG" 2>/dev/null || echo 0)
    ERRORS=$(grep -c "error" "$DBUS_MONITOR_LOG" 2>/dev/null || echo 0)
    
    echo -e "\nTraffic summary:"
    echo "  Method calls: $METHOD_CALLS"
    echo "  Method returns: $METHOD_RETURNS"
    echo "  Errors: $ERRORS"
else
    echo "No dbus-monitor log found or log is empty"
fi

echo -e "\n=== Step 5: Cleanup ==="
echo "Stopping monitor (PID: $MONITOR_PID)..."
kill $MONITOR_PID 2>/dev/null 2>/dev/null
wait $MONITOR_PID 2>/dev/null 2>/dev/null

echo "Stopping server (PID: $SERVER_PID)..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null 2>/dev/null

echo -e "\n=== Test Module Methods Test Complete ==="
echo "Full dbus-monitor log: $DBUS_MONITOR_LOG"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n✓ All tests PASSED!"
    exit 0
else
    echo -e "\n⚠ Some tests FAILED ($FAILED_TESTS/$TOTAL_TESTS)"
    echo "Check $DBUS_MONITOR_LOG for details"
    exit 1
fi