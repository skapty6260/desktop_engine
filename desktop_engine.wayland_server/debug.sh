#!/bin/bash

set -e  # Exit on any error

GDB_TARGET="desktop_engine_wayland_server"

usage() {
    echo "Usage: $0 [gdb-options] -- [server-options]"
    echo ""
    echo "Examples:"
    echo "  $0                                  # Run with default debug settings"
    echo "  $0 -- --log-level info              # Run server with info log level"
    echo "  $0 -- --startup custom-client       # Run with custom startup command"
    echo "  $0 -- --help                        # Show server help"
    echo ""
    echo "Default server args: --log-level debug --no-async --startup test-client"
}

# Check if we should show help
for arg in "$@"; do
    if [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
        usage
        exit 0
    fi
done

# Separate GDB options from server options
GDB_OPTS=()
SERVER_OPTS=("--log-level" "debug" "--no-async" "--startup" "test-client")

# Parse arguments to separate GDB options from server options
while [ $# -gt 0 ]; do
    if [ "$1" = "--" ]; then
        shift
        SERVER_OPTS=("$@")
        break
    else
        GDB_OPTS+=("$1")
        shift
    fi
done

echo "Starting debug session..."
echo "Target: $GDB_TARGET"
echo "Server arguments: ${SERVER_OPTS[*]}"

if [ ${#GDB_OPTS[@]} -gt 0 ]; then
    echo "GDB options: ${GDB_OPTS[*]}"
fi

# Run GDB with the appropriate arguments
gdb "${GDB_OPTS[@]}" "$GDB_TARGET" --args "${SERVER_OPTS[@]}"