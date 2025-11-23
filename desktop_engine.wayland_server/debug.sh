#!/bin/bash

TARGET="desktop_engine_wayland_server"

# Create temporary GDB script
GDB_SCRIPT=$(mktemp)

# Build the run command
if [ $# -eq 0 ]; then
    echo "No arguments provided, using default debug configuration..."
    RUN_CMD="run --log-level debug --no-async --startup test-client"
else
    echo "Running with provided arguments..."
    # Build quoted arguments for GDB
    RUN_CMD="run"
    for arg in "$@"; do
        RUN_CMD="$RUN_CMD \"$arg\""
    done
fi

# Create GDB script
cat > "$GDB_SCRIPT" << EOF
set pagination off
$RUN_CMD
EOF

echo "GDB script content:"
cat "$GDB_SCRIPT"
echo ""

# Run GDB with the script
gdb -x "$GDB_SCRIPT" "$TARGET"

# Clean up
rm -f "$GDB_SCRIPT"