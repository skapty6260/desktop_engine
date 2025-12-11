#!/bin/bash
# service-check.sh - Проверка доступности D-Bus сервиса

echo "=== D-Bus Service Availability Test ==="
echo "Starting server with --no-async --log-level debug..."

# Запускаем сервер
./build/desktop_engine_wayland_server --no-async --log-level debug &
SERVER_PID=$!
echo "Server started with PID: $SERVER_PID"

# Ждем запуска
sleep 3

echo -e "\n=== Step 1: Checking D-Bus daemon ==="
if ! dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
    echo "✗ D-Bus daemon is not available"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi
echo "✓ D-Bus daemon is running"

echo -e "\n=== Step 2: Checking our service ==="
SERVICE_NAME="org.skapty6260.DesktopEngine"

if dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
    echo "✓ Service '$SERVICE_NAME' is available"
else
    echo "✗ Service '$SERVICE_NAME' NOT found"
    echo "Available services:"
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -i desktop || true
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo -e "\n=== Step 3: Testing root path ==="
# Проверяем корневой путь через Introspection
echo "Testing Introspection on root path..."
dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/" \
    org.freedesktop.DBus.Introspectable.Introspect 2>&1 | head -10

echo -e "\n=== Step 4: Cleanup ==="
echo "Stopping server (PID: $SERVER_PID)..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "\n=== Service Availability Test Complete ==="
echo "✓ All service availability checks passed!"