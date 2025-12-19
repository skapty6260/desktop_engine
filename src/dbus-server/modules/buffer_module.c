#include <dbus-server/modules/buffer_module.h>
#include <logger.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

/* Convert pixel_format to string */
const char *pixel_format_to_string(enum pixel_format format) {
    switch (format) {
        case PIXEL_FORMAT_ARGB8888: return "ARGB8888";
        case PIXEL_FORMAT_XRGB8888: return "XRGB8888";
        default: return "UNKNOWN";
    }
}

/* Convert string to pixel_format */
enum pixel_format string_to_pixel_format(const char *str) {
    if (!str) return PIXEL_FORMAT_UNKNOWN;
    if (strcmp(str, "ARGB8888") == 0) return PIXEL_FORMAT_ARGB8888;
    if (strcmp(str, "XRGB8888") == 0) return PIXEL_FORMAT_XRGB8888;
    return PIXEL_FORMAT_UNKNOWN;
}


