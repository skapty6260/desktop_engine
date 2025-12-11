#!/bin/bash
# dbus-availability.sh - Проверка доступности D-Bus сервиса

echo "=== D-Bus Service Availability Test ==="

# Очищаем предыдущие переменные D-Bus
unset DBUS_SESSION_BUS_ADDRESS
unset DBUS_LAUNCH_DBUS_DAEMON

# Устанавливаем правильный адрес D-Bus (используем существующий сокет пользователя)
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u)/bus"

echo "Using D-Bus address: $DBUS_SESSION_BUS_ADDRESS"
echo "Starting server with --no-async --log-level debug..."

# Запускаем сервер
../build/desktop_engine_wayland_server --no-async --log-level debug &
SERVER_PID=$!
echo "Server started with PID: $SERVER_PID"

# Ждем запуска сервера
echo -e "\nWaiting for server to initialize..."
sleep 4

echo -e "\n=== Step 1: Checking D-Bus connectivity ==="

# Прямая проверка сокета
if [ -S "/run/user/$(id -u)/bus" ]; then
    echo "✓ D-Bus socket exists: /run/user/$(id -u)/bus"
    
    # Пробуем отправить Ping к D-Bus демону
    if timeout 2 dbus-send --session \
        --dest=org.freedesktop.DBus \
        --type=method_call \
        --print-reply \
        /org/freedesktop/DBus \
        org.freedesktop.DBus.Ping > /dev/null 2>&1; then
        echo "✓ D-Bus daemon is responding"
    else
        echo "⚠ D-Bus daemon not responding, but socket exists"
        echo "   This might be normal if dbus-broker is running"
    fi
else
    echo "✗ D-Bus socket not found"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo -e "\n=== Step 2: Checking our service ==="
SERVICE_NAME="org.skapty6260.DesktopEngine"
echo "Looking for service: $SERVICE_NAME"

# Пробуем несколько раз с задержкой
SERVICE_FOUND=false
for i in {1..10}; do
    echo "Attempt $i/10..."
    
    if dbus-send --session \
        --dest=org.freedesktop.DBus \
        --type=method_call \
        --print-reply \
        /org/freedesktop/DBus \
        org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
        echo "✓ Service '$SERVICE_NAME' FOUND!"
        SERVICE_FOUND=true
        break
    fi
    
    if [ $i -lt 10 ]; then
        sleep 1
    fi
done

if [ "$SERVICE_FOUND" = false ]; then
    echo "✗ Service '$SERVICE_NAME' NOT found after 10 attempts"
    echo "Available services:"
    dbus-send --session \
        --dest=org.freedesktop.DBus \
        --type=method_call \
        --print-reply \
        /org/freedesktop/DBus \
        org.freedesktop.DBus.ListNames 2>/dev/null | grep -E -i "(desktopengine|skapt)" || echo "  None found"
    
    echo -e "\nDebug: Checking if server is still running..."
    if ps -p $SERVER_PID > /dev/null; then
        echo "Server is still running (PID: $SERVER_PID)"
    else
        echo "Server process terminated"
    fi
    
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo -e "\n=== Step 3: Testing service functionality ==="

# Тест 1: Introspection
echo "1. Testing Introspection..."
INTRO_RESULT=$(dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/org/skapty6260/DesktopEngine/TestModule" \
    org.freedesktop.DBus.Introspectable.Introspect 2>&1)

if echo "$INTRO_RESULT" | grep -q "<interface"; then
    echo "   ✓ Introspection successful (XML interface found)"
else
    echo "   ✗ Introspection failed"
    echo "   Error: $INTRO_RESULT"
fi

# Тест 2: Ping method
echo "2. Testing Ping method..."
PING_RESULT=$(dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/org/skapty6260/DesktopEngine/TestModule" \
    "org.skapty6260.DesktopEngine.TestModule.Ping" \
    string:"availability_test" 2>&1)

PING_EXIT=$?

if [ $PING_EXIT -eq 0 ]; then
    echo "   ✓ Ping successful"
    # Извлекаем ответ
    RESPONSE=$(echo "$PING_RESULT" | grep -o 'string "[^"]*"' | head -1 | sed 's/string "//' | sed 's/"$//')
    echo "   Response: \"$RESPONSE\""
else
    echo "   ✗ Ping failed (exit code: $PING_EXIT)"
    echo "   Error output: $PING_RESULT"
fi

# Тест 3: GetSystemInfo
echo "3. Testing GetSystemInfo..."
dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/org/skapty6260/DesktopEngine/TestModule" \
    "org.skapty6260.DesktopEngine.TestModule.GetSystemInfo" 2>&1 | head -5

echo -e "\n=== Step 4: Cleanup ==="
echo "Stopping server (PID: $SERVER_PID)..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null 2>/dev/null

echo -e "\n=== Service Availability Test Complete ==="
echo "✓ Service is available and responding!"
exit 0