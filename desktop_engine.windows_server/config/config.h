#ifndef CONFIG_H
#define CONFIG_H

#include "logger/logger.h"
#include "src/server.h"

void parse_args(int argc, char **argv);

/* Read logger config from file */
int logger_config_read(const char* config_path, logger_config_t* config);
/* Load default logger config */
void logger_config_default(logger_config_t* config);

#endif