#!/bin/bash

# Simple D-Bus Ping test script
# Sends a single Ping method call to the test module

# Configuration
SERVICE_NAME="org.skaptg6260.DesktopEngine"
OBJECT_PATH="/org/skaptg6260/DesktopEngine/TestModule"
INTERFACE="org.skaptg6260.DesktopEngine.TestModule"
METHOD="Ping"
MESSAGE="Test from simple script"

echo "=== Simple D-Bus Ping Test ==="
echo "Service: $SERVICE_NAME"
echo "Object: $OBJECT_PATH"
echo "Interface: $INTERFACE"
echo "Method: $METHOD"
echo "Message: $MESSAGE"
echo ""

# Check if D-Bus daemon is available
if ! dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.Ping > /dev/null 2>&1; then
    echo "ERROR: D-Bus daemon is not available"
    exit 1
fi

# Check if our service is available
echo "Checking service availability..."
if dbus-send --session --dest=org.freedesktop.DBus \
    --type=method_call --print-reply \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "$SERVICE_NAME"; then
    echo "✓ Service '$SERVICE_NAME' is available"
else
    echo "✗ Service '$SERVICE_NAME' NOT found"
    echo "Available services containing 'skapt':"
    dbus-send --session --dest=org.freedesktop.DBus \
        --type=method_call --print-reply \
        /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -i skapt
    exit 1
fi

# Send the Ping method
echo -e "\nSending Ping method..."
echo "Command: dbus-send --session --dest=$SERVICE_NAME --type=method_call --print-reply \\"
echo "  $OBJECT_PATH $INTERFACE.$METHOD string:'$MESSAGE'"

# Execute the Ping
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
    echo "✓ Ping successful!"
    echo "Response:"
    echo "$RESULT" | sed 's/^/  /'
else
    echo "✗ Ping failed with exit code: $EXIT_CODE"
    echo "Error output:"
    echo "$RESULT" | sed 's/^/  /'
fi

echo -e "\n=== Test complete ==="