#ifndef CONFIG_H
#define CONFIG_H

#include <logger.h>
#include <wayland/server.h>

void parse_args(int argc, char **argv, logger_config_t *logger_config, server_config_t *server_config);
void load_default_config(logger_config_t* logger_conf, server_config_t* server_config);

/* Read logger config from file */
int logger_config_read(const char* config_path, logger_config_t* config);
/* Load default server config */
void server_config_default(server_config_t* config);

#endif