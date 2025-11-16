/* TODO
Basic wayland server (Open clients, create surfaces)
Logger (Should work with logfiles, ipc logging)
Ipc bridge (Should send clients info, should receive input events)
*/
#include "logger/logger.h"
#include "logger/logger_config.h"
#include "server.h"
#include <unistd.h>

static struct server_config {
    const char* startup_cmd;
};

void parse_args(int argc, char **argv, logger_config_t *logger_config, struct server_config *server_config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--log-config") == 0 && i + 1 < argc) {
            logger_config_read(argv[++i], config);
        }
        else if (strcmp(argv[i], "--startup") == 0 && i + 1 < argc) {
            strncpy(server_config->startup_cmd, argv[++i], sizeof(server_config->startup_cmd) - 1);
        } 
        else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            config->level = parse_log_level(argv[++i]);
        }
        else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            strncpy(config->log_file_path, argv[++i], sizeof(config->log_file_path) - 1);
            config->log_to_file = 1;
        }
        else if (strcmp(argv[i], "--no-colors") == 0) {
            config->use_colors = 0;
        }
        else if (strcmp(argv[i], "--no-async") == 0) {
            config->async_enabled = 0;
        }
        else if (strcmp(argv[i], "--console-only") == 0) {
            config->log_to_file = 0;
        }
        else if (strcmp(argv[i], "--file-only") == 0) {
            config->log_to_console = 0;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --startup COMMAND   Startup command for server\n");
            printf("  --log-config FILE   Load configuration from file\n");
            printf("  --log-level LEVEL   Set log level (debug, info, warn, error, fatal)\n");
            printf("  --log-file FILE     Log to specified file\n");
            printf("  --no-colors         Disable colored output\n");
            printf("  --no-async          Disable asynchronous logging\n");
            printf("  --console-only      Log only to console\n");
            printf("  --file-only         Log only to file\n");
            printf("  --help              Show this help message\n");
            exit(0);
        }
    }
}

int main(int argc, char **argv) {
    logger_config_t logger_config;
    struct server_config server_config;
    struct server server = {0};
    
    /* Logger initialization */
    logger_config_default(&logger_config);
    parse_args(argc, argv, &logger_config, &server_config);
    if (!logger_init(&config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    /* Wayland server initialization */
    // struct server *server = calloc(1, sizeof(struct server));
    // if (!server) {
    //     LOG_FATAL(LOG_MODULE_CORE, "Failed to allocate memory for wayland server");
    // }

    if (!getenv("XDG_RUNTIME_DIR")) {
        SERVER_FATAL("XDG_RUNTIME_DIR is not set in the environment. Aborting.");
    }

    printf("Startup command %s\n", server_config.startup_cmd);
    server_init(&server);
    server_run(&server);
    
    LOG_FATAL(LOG_MODULE_CORE, "Testing fatal");

    LOG_INFO(LOG_MODULE_CORE, "Exiting windows server");
    
    logger_cleanup();

    return 0;
}