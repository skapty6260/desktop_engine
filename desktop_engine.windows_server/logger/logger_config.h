#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

#include "logger.h"

// Чтение конфигурации из файла
int logger_config_read(const char* config_path, logger_config_t* config);
// Парсинг аргументов командной строки
void logger_config_parse_args(int argc, char* argv[], logger_config_t* config);
// Конфигурация по умолчанию
void logger_config_default(logger_config_t* config);

#endif