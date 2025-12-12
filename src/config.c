#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

static void log_help(char **argv) {
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

void load_default_config(logger_config_t* logger_config, server_config_t* server_config) {
    /* Logger config*/
    logger_config->level = LOG_LEVEL_INFO;
    logger_config->use_colors = 1;
    logger_config->log_to_file = 0;
    logger_config->log_to_console = 1;
    logger_config->async_enabled = 1;
    logger_config->flush_immediately = 0;
    strcpy(logger_config->log_file_path, "application.log");
    /* Server config */
    server_config->startup_cmd = NULL;
}

static log_level_t parse_log_level(const char* level_str) {
    if (strcmp(level_str, "debug") == 0) return LOG_LEVEL_DEBUG;
    if (strcmp(level_str, "info") == 0) return LOG_LEVEL_INFO;
    if (strcmp(level_str, "warn") == 0) return LOG_LEVEL_WARN;
    if (strcmp(level_str, "error") == 0) return LOG_LEVEL_ERROR;
    if (strcmp(level_str, "fatal") == 0) return LOG_LEVEL_FATAL;
    return LOG_LEVEL_INFO; // По умолчанию
}

static int parse_boolean(const char* value) {
    if (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || 
        strcmp(value, "1") == 0 || strcmp(value, "on") == 0) {
        return 1;
    }
    return 0;
}

static void trim_string(char* str) {
    char* end;
    
    // Удаление пробелов в начале
    while (isspace((unsigned char)*str)) str++;
    
    // Удаление пробелов в конце
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Запись нулевого терминатора
    *(end + 1) = '\0';
}

int logger_config_read(const char* config_path, logger_config_t* config) {
    FILE* file = fopen(config_path, "r");
    if (!file) {
        return 0;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Пропуск комментариев и пустых строк
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Разделение ключа и значения
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        
        if (key && value) {
            trim_string(key);
            trim_string(value);
            
            if (strcmp(key, "log_level") == 0) {
                config->level = parse_log_level(value);
            }
            else if (strcmp(key, "use_colors") == 0) {
                config->use_colors = parse_boolean(value);
            }
            else if (strcmp(key, "log_to_file") == 0) {
                config->log_to_file = parse_boolean(value);
            }
            else if (strcmp(key, "log_to_console") == 0) {
                config->log_to_console = parse_boolean(value);
            }
            else if (strcmp(key, "log_file_path") == 0) {
                strncpy(config->log_file_path, value, sizeof(config->log_file_path) - 1);
            }
            else if (strcmp(key, "async_enabled") == 0) {
                config->async_enabled = parse_boolean(value);
            }
            else if (strcmp(key, "flush_immediately") == 0) {
                config->flush_immediately = parse_boolean(value);
            }
        }
    }
    
    fclose(file);
    return 1;
}

void parse_args(int argc, char **argv, logger_config_t *logger_config, server_config_t *server_config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--log-config") == 0 && i + 1 < argc) {
            logger_config_read(argv[++i], logger_config);
        }
        else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            logger_config->level = parse_log_level(argv[++i]);
        }
        else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            strncpy(logger_config->log_file_path, argv[++i], sizeof(logger_config->log_file_path) - 1);
            logger_config->log_to_file = 1;
        }
        else if (strcmp(argv[i], "--no-colors") == 0) {
            logger_config->use_colors = 0;
        }
        else if (strcmp(argv[i], "--no-async") == 0) {
            logger_config->async_enabled = 0;
        }
        else if (strcmp(argv[i], "--console-only") == 0) {
            logger_config->log_to_file = 0;
        }
        else if (strcmp(argv[i], "--file-only") == 0) {
            logger_config->log_to_console = 0;
        }
        else if (strcmp(argv[i], "--startup") == 0) {
            if (i + 1 < argc) {
                i++;
                server_config->startup_cmd = argv[i];
            } else {
                fprintf(stderr, "Error: --startup requires an argument\n");
                exit(1);
            }
        } 
        else if (strcmp(argv[i], "--help") == 0) {
            log_help(argv);
        }
    }
}