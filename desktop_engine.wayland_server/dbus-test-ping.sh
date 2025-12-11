#!/bin/bash
# Простой скрипт который точно работает с dbus-broker

# Устанавливаем правильный адрес D-Bus
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u)/bus"

# Отправляем Ping
dbus-send --session \
    --dest="org.skaptg6260.DesktopEngine" \
    --type=method_call \
    --print-reply \
    "/org/skaptg6260/DesktopEngine/TestModule" \
    "org.skaptg6260.DesktopEngine.TestModule.Ping" \
    string:"test"

echo "Exit code: $?"