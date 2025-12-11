#!/bin/bash

# Simple D-Bus Ping test script
# Работает с dbus-broker через systemd

echo "=== D-Bus Ping Test (with dbus-broker) ==="

# Configuration
SERVICE_NAME="org.skaptg6260.DesktopEngine"
OBJECT_PATH="/org/skaptg6260/DesktopEngine/TestModule"
INTERFACE="org.skaptg6260.DesktopEngine.TestModule"
METHOD="Ping"
MESSAGE="Test from script"

# 1. Получаем адрес D-Bus сессии из systemd
echo "1. Getting D-Bus session address from systemd..."
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    # Получаем адрес из systemd
    DBUS_SESSION_BUS_ADDRESS=$(systemctl --user show-environment | grep DBUS_SESSION_BUS_ADDRESS | cut -d= -f2)
    
    if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
        # Пробуем получить адрес из сокета
        if [ -n "$XDG_RUNTIME_DIR" ] && [ -S "$XDG_RUNTIME_DIR/bus" ]; then
            DBUS_SESSION_BUS_ADDRESS="unix:path=$XDG_RUNTIME_DIR/bus"
            echo "   Using socket: $XDG_RUNTIME_DIR/bus"
        else
            # Ищем сокет вручную
            SOCKET_PATH="/run/user/$(id -u)/bus"
            if [ -S "$SOCKET_PATH" ]; then
                DBUS_SESSION_BUS_ADDRESS="unix:path=$SOCKET_PATH"
                echo "   Found socket: $SOCKET_PATH"
            else
                echo "   ERROR: Cannot find D-Bus session address"
                exit 1
            fi
        fi
    fi
    
    export DBUS_SESSION_BUS_ADDRESS
    echo "   DBUS_SESSION_BUS_ADDRESS set to: $DBUS_SESSION_BUS_ADDRESS"
else
    echo "   DBUS_SESSION_BUS_ADDRESS already set: $DBUS_SESSION_BUS_ADDRESS"
fi

# 2. Проверяем подключение к D-Bus
echo -e "\n2. Testing D-Bus connection..."
if dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
    echo "   ✓ D-Bus connection successful"
else
    echo "   ✗ Cannot connect to D-Bus"
    echo "   Trying alternative method..."
    
    # Пробуем напрямую через сокет
    if [ -S "${XDG_RUNTIME_DIR}/bus" ]; then
        DBUS_SESSION_BUS_ADDRESS="unix:path=${XDG_RUNTIME_DIR}/bus"
        export DBUS_SESSION_BUS_ADDRESS
        echo "   Retrying with direct socket..."
    fi
    
    # Финальная проверка
    if ! dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
        echo "   ERROR: D-Bus connection failed"
        exit 1
    fi
    echo "   ✓ D-Bus connection established"
fi

# 3. Проверяем наличие нашего сервиса
echo -e "\n3. Checking for service: $SERVICE_NAME"
if dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -q "$SERVICE_NAME"; then
    echo "   ✓ Service found"
else
    echo "   ✗ Service NOT found"
    echo "   Listing all services with 'skapt':"
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep -i skapt || echo "   None found"
    exit 1
fi

# 4. Отправляем Ping
echo -e "\n4. Sending Ping to $SERVICE_NAME"
echo "   Object: $OBJECT_PATH"
echo "   Interface: $INTERFACE.$METHOD"
echo "   Message: \"$MESSAGE\""

RESULT=$(dbus-send --session \
    --dest="$SERVICE_NAME" \
    --type=method_call \
    --print-reply \
    "$OBJECT_PATH" \
    "$INTERFACE.$METHOD" \
    string:"$MESSAGE" 2>&1)

EXIT_CODE=$?

echo -e "\n=== Result ==="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ SUCCESS: Ping received response"
    echo "Response:"
    echo "$RESULT" | sed 's/^/  /'
else
    echo "✗ FAILED: Exit code $EXIT_CODE"
    echo "Error:"
    echo "$RESULT" | sed 's/^/  /'
    
    # Дополнительная диагностика
    echo -e "\n=== Debug Info ==="
    echo "Trying introspection..."
    dbus-send --session \
        --dest="$SERVICE_NAME" \
        --type=method_call \
        --print-reply \
        "$OBJECT_PATH" \
        org.freedesktop.DBus.Introspectable.Introspect 2>&1 | head -20
fi

echo -e "\n=== Test Complete ==="