#!/bin/bash

TARGET="desktop_engine_wayland_server"
DEFAULT_ARGS=("--log-level" "debug" "--no-async" "--startup" "test-client")

# Function to find the executable
find_executable() {
    local target="$1"
    
    # Check if it's in current directory
    if [ -f "./$target" ] && [ -x "./$target" ]; then
        echo "./$target"
        return 0
    fi
    
    # Check if it's in PATH
    if command -v "$target" > /dev/null 2>&1; then
        echo "$target"
        return 0
    fi
    
    return 1
}

# Find the executable
EXECUTABLE=$(find_executable "$TARGET")
if [ $? -ne 0 ]; then
    echo "Error: Executable '$TARGET' not found"
    echo "Please ensure it exists in the current directory or is in your PATH"
    exit 1
fi

# Run with appropriate arguments
if [ $# -eq 0 ]; then
    echo "No arguments provided, using default configuration..."
    echo "Running: $EXECUTABLE ${DEFAULT_ARGS[*]}"
    "$EXECUTABLE" "${DEFAULT_ARGS[@]}"
else
    echo "Running with provided arguments..."
    echo "Command: $EXECUTABLE $*"
    "$EXECUTABLE" "$@"
fi