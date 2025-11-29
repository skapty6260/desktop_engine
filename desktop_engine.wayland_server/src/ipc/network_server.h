#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include <server/wayland/buffer.h>
#include <stdint.h>
#include <stdbool.h>

#define PORT 8000
#define MAX_CLIENTS 10

struct network_header {
    uint32_t magic;          // Магическое число для проверки
    uint32_t type;           // Тип пакета
    uint32_t size;           // Размер данных
    uint32_t checksum;       // Контрольная сумма
};

enum packet_type {
    PACKET_TYPE_BUFFER = 1,
    PACKET_TYPE_PING = 2,
    PACKET_TYPE_PONG = 3
};

#define NETWORK_MAGIC 0xDEADBEEF

int network_server_init(int port);
void network_server_cleanup(void);
bool network_server_send_buffer(struct buffer *buf, int client_fd);
void network_server_broadcast_buffer(struct buffer *buf);
int network_server_get_clients(int *client_fds, int max_clients);
void network_server_run(void);  // Блокирующая функция для запуска сервера

#endif