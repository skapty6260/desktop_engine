#include <wayland-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    const char *socket_name = getenv("WAYLAND_DISPLAY");
    if (!socket_name) {
        socket_name = "wayland-0"; // default socket name
    }
    
    printf("Connecting to Wayland display: %s\n", socket_name);
    
    // Connect to Wayland display
    struct wl_display *display = wl_display_connect(socket_name);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display: %s\n", socket_name);
        return 1;
    }
    
    printf("Successfully connected to Wayland display\n");
    printf("Display fd: %d\n", wl_display_get_fd(display));
    
    // Get the display version (just to demonstrate minimal interaction)
    int version = wl_display_get_version(display);
    printf("Wayland protocol version: %d\n", version);
    
    // Do a single roundtrip to ensure connection is working
    wl_display_roundtrip(display);
    printf("Roundtrip completed - connection is active\n");
    
    printf("Client will sleep for 5 seconds to demonstrate connection...\n");
    sleep(5);
    
    printf("Disconnecting from Wayland display\n");
    wl_display_disconnect(display);
    
    return 0;
}