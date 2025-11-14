/* TODO
Basic wayland server (Open clients, create surfaces)
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include <stdio.h>
#include <server.h>
#include "logger/logger.h"

int main(void) {
    log_message(LOG_LEVEL_INFO, "App started.");

    return 0;
}