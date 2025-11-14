#include "logger_config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Конфигурация по умолчанию
void logger_config_default(logger_config_t* config) {
    config->level = LOG_LEVEL_INFO;
    config->use_colors = 1;
    config->log_to_file = 0;
    config->log_to_console = 1;
    config->async_enabled = 1;
    config->flush_immediately = 0;
    strcpy(config->log_file_path, "application.log");
}

// Парсинг уровня логирования из строки
static log_level_t parse_log_level(const char* level_str) {
    if (strcasecmp(level_str, "debug") == 0) return LOG_LEVEL_DEBUG;
    if (strcasecmp(level_str, "info") == 0) return LOG_LEVEL_INFO;
    if (strcasecmp(level_str, "warn") == 0) return LOG_LEVEL_WARN;
    if (strcasecmp(level_str, "error") == 0) return LOG_LEVEL_ERROR;
    if (strcasecmp(level_str, "fatal") == 0) return LOG_LEVEL_FATAL;
    return LOG_LEVEL_INFO; // По умолчанию
}

// Парсинг булевого значения
static int parse_boolean(const char* value) {
    if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || 
        strcasecmp(value, "1") == 0 || strcasecmp(value, "on") == 0) {
        return 1;
    }
    return 0;
}

// Удаление пробелов в начале и конце строки
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

// Чтение конфигурации из файла
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
            
            if (strcasecmp(key, "log_level") == 0) {
                config->level = parse_log_level(value);
            }
            else if (strcasecmp(key, "use_colors") == 0) {
                config->use_colors = parse_boolean(value);
            }
            else if (strcasecmp(key, "log_to_file") == 0) {
                config->log_to_file = parse_boolean(value);
            }
            else if (strcasecmp(key, "log_to_console") == 0) {
                config->log_to_console = parse_boolean(value);
            }
            else if (strcasecmp(key, "log_file_path") == 0) {
                strncpy(config->log_file_path, value, sizeof(config->log_file_path) - 1);
            }
            else if (strcasecmp(key, "async_enabled") == 0) {
                config->async_enabled = parse_boolean(value);
            }
            else if (strcasecmp(key, "flush_immediately") == 0) {
                config->flush_immediately = parse_boolean(value);
            }
        }
    }
    
    fclose(file);
    return 1;
}

// Парсинг аргументов командной строки
void logger_config_parse_args(int argc, char* argv[], logger_config_t* config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            logger_config_read(argv[++i], config);
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
            printf("  --config FILE       Load configuration from file\n");
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