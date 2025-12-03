#include "network_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

static int server_fd = -1;
static int client_fds[MAX_CLIENTS];
static int num_clients = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Вычисление простой контрольной суммы
static uint32_t calculate_checksum(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

// Сериализация buffer в сетевой формат
static bool serialize_buffer(const struct buffer *buf, void **data, size_t *size) {
    // Вычисляем общий размер
    size_t total_size = sizeof(struct network_header) + sizeof(struct buffer);
    
    // Для SHM буферов добавляем размер данных
    if (buf->type == WL_BUFFER_SHM && buf->shm.data) {
        total_size += buf->size;
    }
    
    *data = malloc(total_size);
    if (!*data) return false;
    
    // Заполняем заголовок
    struct network_header *header = (struct network_header *)*data;
    header->magic = htonl(NETWORK_MAGIC);
    header->type = htonl(PACKET_TYPE_BUFFER);
    header->size = htonl(total_size - sizeof(struct network_header));
    header->checksum = 0; // Пока 0
    
    // Копируем структуру buffer (после заголовка)
    struct buffer *net_buf = (struct buffer *)(header + 1);
    memcpy(net_buf, buf, sizeof(struct buffer));
    
    // Обнуляем указатели и файловые дескрипторы для сети
    net_buf->resource = NULL;
    net_buf->shm.data = NULL;
    
    // Для SHM буферов копируем данные
    if (buf->type == WL_BUFFER_SHM && buf->shm.data) {
        void *buffer_data = (void *)(net_buf + 1);
        memcpy(buffer_data, buf->shm.data, buf->size);
        // В сетевой структуре data указывает на смещение
        net_buf->shm.data = (void *)sizeof(struct buffer);
    }
    
    // Вычисляем контрольную сумму
    header->checksum = htonl(calculate_checksum(net_buf, total_size - sizeof(struct network_header)));
    
    *size = total_size;
    return true;
}

// Отправка данных клиенту
static bool send_data(int client_fd, const void *data, size_t size) {
    size_t total_sent = 0;
    
    while (total_sent < size) {
        ssize_t sent = send(client_fd, (const char *)data + total_sent, 
                           size - total_sent, MSG_NOSIGNAL);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return false;
        }
        total_sent += sent;
    }
    
    return true;
}

// Добавление клиента
static void add_client(int client_fd) {
    pthread_mutex_lock(&clients_mutex);
    
    if (num_clients < MAX_CLIENTS) {
        client_fds[num_clients++] = client_fd;
        printf("Client connected: fd=%d, total clients: %d\n", client_fd, num_clients);
    } else {
        close(client_fd);
        printf("Max clients reached, rejecting connection\n");
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Удаление клиента
static void remove_client(int client_fd) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < num_clients; i++) {
        if (client_fds[i] == client_fd) {
            // Сдвигаем массив
            for (int j = i; j < num_clients - 1; j++) {
                client_fds[j] = client_fds[j + 1];
            }
            num_clients--;
            printf("Client disconnected: fd=%d, remaining clients: %d\n", client_fd, num_clients);
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Обработка клиента в отдельном потоке
static void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    
    add_client(client_fd);
    
    // Простой echo-сервер для демонстрации
    char buffer[1024];
    while (1) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            break;
        }
        
        // Эхо-ответ (можно убрать)
        send(client_fd, buffer, bytes_read, 0);
    }
    
    remove_client(client_fd);
    close(client_fd);
    return NULL;
}

// Инициализация сервера
int network_server_init(int port) {
    struct sockaddr_in address;
    int opt = 1;
    
    // Создаем сокет
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return -1;
    }
    
    // Настраиваем опции сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Биндим сокет
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Слушаем входящие соединения
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    
    printf("Network server started on port %d\n", port);
    return 0;
}

// Отправка buffer конкретному клиенту
bool network_server_send_buffer(struct buffer *buf, int client_fd) {
    void *data;
    size_t size;
    
    if (!serialize_buffer(buf, &data, &size)) {
        return false;
    }
    
    bool result = send_data(client_fd, data, size);
    free(data);
    return result;
}

// Broadcast buffer всем подключенным клиентам
void network_server_broadcast_buffer(struct buffer *buf) {
    void *data;
    size_t size;

    printf("Broadcasting buffer to %d clients\n", num_clients);
    
    if (!serialize_buffer(buf, &data, &size)) {
        return;
    }
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < num_clients; i++) {
        if (!send_data(client_fds[i], data, size)) {
            // Ошибка отправки - помечаем клиента для удаления
            printf("Failed to send to client %d\n", client_fds[i]);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    free(data);
}

// Получение списка клиентов
int network_server_get_clients(int *client_fds_out, int max_clients) {
    pthread_mutex_lock(&clients_mutex);
    
    int count = (num_clients < max_clients) ? num_clients : max_clients;
    memcpy(client_fds_out, client_fds, count * sizeof(int));
    
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

// Запуск сервера (блокирующий)
void network_server_run(void) {
    if (server_fd == -1) {
        fprintf(stderr, "Server not initialized\n");
        return;
    }
    
    printf("Server listening for connections...\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Создаем поток для обработки клиента
        pthread_t thread_id;
        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client_fd;
        
        if (pthread_create(&thread_id, NULL, client_handler, client_ptr) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(client_ptr);
        } else {
            pthread_detach(thread_id);
        }
    }
}

// Очистка ресурсов
void network_server_cleanup(void) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < num_clients; i++) {
        close(client_fds[i]);
    }
    num_clients = 0;
    
    pthread_mutex_unlock(&clients_mutex);
    
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    
    printf("Network server stopped\n");
}