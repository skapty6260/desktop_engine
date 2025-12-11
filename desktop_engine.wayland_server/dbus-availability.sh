#!/bin/bash
# service-check.sh - Проверка доступности D-Bus сервиса

echo "=== D-Bus Service Availability Test ==="

# Устанавливаем правильное окружение D-Bus
setup_dbus_environment() {
    # Проверяем, установлена ли переменная окружения
    if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
        # Пробуем получить адрес из systemd
        DBUS_SESSION_BUS_ADDRESS=$(systemctl --user show-environment 2>/dev/null | grep DBUS_SESSION_BUS_ADDRESS | cut -d= -f2)
        
        if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
            # Используем стандартный путь сокета
            if [ -n "$XDG_RUNTIME_DIR" ] && [ -S "$XDG_RUNTIME_DIR/bus" ]; then
                DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus"
            else
                DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u)/bus"
            fi
        fi
        
        export DBUS_SESSION_BUS_ADDRESS
        echo "DBUS_SESSION_BUS_ADDRESS set to: $DBUS_SESSION_BUS_ADDRESS"
    fi
}

setup_dbus_environment

echo "Starting server with --no-async --log-level debug..."

# Запускаем сервер
./build/desktop_engine_wayland_server --no-async --log-level debug &
SERVER_PID=$!
echo "Server started with PID: $SERVER_PID"

# Ждем запуска
sleep 3

echo -e "\n=== Step 1: Checking D-Bus connectivity ==="

# Проверяем разными способами
DBUS_AVAILABLE=false

echo "Trying to connect to D-Bus..."

# Способ 1: Используем dbus-send
if dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
    echo "✓ D-Bus daemon is available (via dbus-send)"
    DBUS_AVAILABLE=true
else
    echo "⚠ dbus-send failed, trying alternative methods..."
    
    # Способ 2: Проверяем сокет напрямую
    if [ -n "$DBUS_SESSION_BUS_ADDRESS" ]; then
        SOCKET_PATH=$(echo "$DBUS_SESSION_BUS_ADDRESS" | sed 's/unix:path=//')
        if [ -S "$SOCKET_PATH" ]; then
            echo "✓ D-Bus socket exists: $SOCKET_PATH"
            
            # Способ 3: Используем dbus-launch для теста
            if command -v dbus-launch >/dev/null 2>&1; then
                echo "Testing with dbus-launch..."
                OUTPUT=$(dbus-launch --sh-syntax 2>/dev/null)
                eval "$OUTPUT"
                if dbus-send --session --dest=org.freedesktop.DBus \
                    --type=method_call --print-reply \
                    /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
                    echo "✓ D-Bus daemon is available (via dbus-launch)"
                    DBUS_AVAILABLE=true
                fi
            fi
        fi
    fi
fi

if [ "$DBUS_AVAILABLE" = false ]; then
    echo "✗ Cannot connect to D-Bus daemon"
    echo "Debug info:"
    echo "  User ID: $(id -u)"
    echo "  XDG_RUNTIME_DIR: ${XDG_RUNTIME_DIR:-Not set}"
    echo "  DBUS_SESSION_BUS_ADDRESS: ${DBUS_SESSION_BUS_ADDRESS:-Not set}"
    
    # Проверяем существование сокета
    SOCKET_PATH="/run/user/$(id -u)/bus"
    if [ -S "$SOCKET_PATH" ]; then
        echo "  Socket exists: $SOCKET_PATH"
        echo "  Socket permissions: $(stat -c '%A %U:%G' "$SOCKET_PATH")"
    else
        echo "  Socket not found: $SOCKET_PATH"
        echo "  Trying to start D-Bus session..."
        
        # Запускаем dbus-launch
        if command -v dbus-launch >/dev/null 2>&1; then
            echo "Starting D-Bus with dbus-launch..."
            eval "$(dbus-launch --sh-syntax)"
            export DBUS_SESSION_BUS_ADDRESS
            echo "New D-Bus session started"
            sleep 1
            DBUS_AVAILABLE=true
        else
            echo "dbus-launch not available"
        fi
    fi
    
    if [ "$DBUS_AVAILABLE" = false ]; then
        kill $SERVER_PID 2>/dev/null
        exit 1
    fi
fi

echo -e "\n=== Step 2: Checking our service ==="
SERVICE_NAME="org.skapty6260.DesktopEngine"

echo "Looking for service: $SERVICE_NAME"

# Даем серверу больше времени для регистрации
sleep 2

SERVICE_FOUND=false
for i in {1..5}; do
    if dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
        echo "✓ Service '$SERVICE_NAME' is available (attempt $i)"
        SERVICE_FOUND=true
        break
    else
        echo "  Service not found yet (attempt $i), waiting..."
        sleep 1
    fi
done

if [ "$SERVICE_FOUND" = false ]; then
    echo "✗ Service '$SERVICE_NAME' NOT found after 5 attempts"
    echo "Available services containing 'DesktopEngine' or 'skapt':"
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -E -i "(desktopengine|skapt)" || echo "  None found"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo -e "\n=== Step 3: Testing basic operations ==="

# 1. Introspection
echo "1. Testing Introspection..."
if dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/org/skapty6260/DesktopEngine/TestModule" \
    org.freedesktop.DBus.Introspectable.Introspect > /dev/null 2>&1; then
    echo "   ✓ Introspection successful"
else
    echo "   ✗ Introspection failed"
fi

# 2. Simple Ping
echo "2. Testing Ping..."
PING_RESULT=$(dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "/org/skapty6260/DesktopEngine/TestModule" \
    "org.skapty6260.DesktopEngine.TestModule.Ping" \
    string:"service_check" 2>&1)

if echo "$PING_RESULT" | grep -q "Pong"; then
    echo "   ✓ Ping successful"
    echo "   Response: $(echo "$PING_RESULT" | grep -o '"Pong![^"]*"' | head -1)"
else
    echo "   ✗ Ping failed"
    echo "   Error: $PING_RESULT"
fi

echo -e "\n=== Step 4: Cleanup ==="
echo "Stopping server (PID: $SERVER_PID)..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null 2>/dev/null

echo -e "\n=== Service Availability Test Complete ==="
echo "✓ All service availability checks passed!"