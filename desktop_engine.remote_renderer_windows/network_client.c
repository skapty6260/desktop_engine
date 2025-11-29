#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <signal.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8000
#define RECONNECT_DELAY 3000  // 3 seconds

// Структуры должны совпадать с сервером
struct network_header {
    uint32_t magic;
    uint32_t type;
    uint32_t size;
    uint32_t checksum;
};

enum packet_type {
    PACKET_TYPE_BUFFER = 1,
    PACKET_TYPE_PING = 2,
    PACKET_TYPE_PONG = 3
};

enum pixel_format {
    PIXEL_FORMAT_UNKNOWN = 0,
    PIXEL_FORMAT_ARGB8888,
    PIXEL_FORMAT_XRGB8888,
    PIXEL_FORMAT_ABGR8888,
    PIXEL_FORMAT_XBGR8888,
    PIXEL_FORMAT_RGBA8888,
    PIXEL_FORMAT_BGRA8888,
    PIXEL_FORMAT_RGB565,
    PIXEL_FORMAT_BGR565,
    PIXEL_FORMAT_NV12,
    PIXEL_FORMAT_NV21,
    PIXEL_FORMAT_YUV420,
    PIXEL_FORMAT_YVU420,
};

struct buffer {
    uint32_t width, height;
    uint32_t type;  // WL_BUFFER_SHM, WL_BUFFER_DMA_BUF, etc.
    enum pixel_format format;
    size_t size;
    // Другие поля в зависимости от типа буфера
    union {
        struct {
            uint32_t stride;
            size_t data_offset;  // Смещение данных в пакете
        } shm;
        struct {
            uint32_t strides[4];
            uint32_t offsets[4];
            uint64_t modifier;
            uint32_t num_planes;
        } dmabuf;
    };
};

#define NETWORK_MAGIC 0xDEADBEEF

static volatile int g_running = 1;

// Обработчик Ctrl+C
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        printf("\nCtrl+C received. Shutting down...\n");
        g_running = 0;
        return TRUE;
    }
    return FALSE;
}

// Функция для проверки контрольной суммы
static uint32_t calculate_checksum(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

// Функция для преобразования big-endian в little-endian (если нужно)
static uint32_t ntohl_win(uint32_t netlong) {
    return ((netlong & 0xFF) << 24) | 
           ((netlong & 0xFF00) << 8) | 
           ((netlong & 0xFF0000) >> 8) | 
           ((netlong & 0xFF000000) >> 24);
}

// Десериализация буфера
static void deserialize_buffer(const void *data, struct buffer *buf) {
    const struct network_header *header = (const struct network_header *)data;
    const struct buffer *net_buf = (const struct buffer *)(header + 1);
    
    // Копируем основную структуру
    memcpy(buf, net_buf, sizeof(struct buffer));
    
    // Преобразуем endianness если нужно
    buf->width = ntohl_win(buf->width);
    buf->height = ntohl_win(buf->height);
    buf->size = ntohl_win((uint32_t)buf->size);
    
    printf("Received buffer: %dx%d, type=%d, format=%d, size=%zu\n", 
           buf->width, buf->height, buf->type, buf->format, buf->size);
}

// Функция для получения строкового представления формата
static const char* format_to_string(enum pixel_format format) {
    switch (format) {
        case PIXEL_FORMAT_ARGB8888: return "ARGB8888";
        case PIXEL_FORMAT_XRGB8888: return "XRGB8888";
        case PIXEL_FORMAT_ABGR8888: return "ABGR8888";
        case PIXEL_FORMAT_XBGR8888: return "XBGR8888";
        case PIXEL_FORMAT_RGBA8888: return "RGBA8888";
        case PIXEL_FORMAT_BGRA8888: return "BGRA8888";
        case PIXEL_FORMAT_RGB565: return "RGB565";
        case PIXEL_FORMAT_BGR565: return "BGR565";
        case PIXEL_FORMAT_NV12: return "NV12";
        case PIXEL_FORMAT_NV21: return "NV21";
        case PIXEL_FORMAT_YUV420: return "YUV420";
        case PIXEL_FORMAT_YVU420: return "YVU420";
        default: return "UNKNOWN";
    }
}

// Функция для подключения к серверу
SOCKET connect_to_server() {
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    
    // Создаем сокет
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }
    
    // Настраиваем адрес сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Подключаемся к серверу
    printf("Connecting to server %s:%d...", SERVER_IP, SERVER_PORT);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf(" failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        return INVALID_SOCKET;
    }
    
    printf(" success!\n");
    return sockfd;
}

// Основная функция работы с сервером
void run_client(SOCKET sockfd) {
    char recv_buffer[8192];
    int buffer_count = 0;
    
    printf("Waiting for buffers...\n");
    
    while (g_running) {
        // Сначала читаем заголовок
        int header_read = recv(sockfd, recv_buffer, sizeof(struct network_header), 0);
        if (header_read <= 0) {
            if (header_read == 0) {
                printf("Server disconnected\n");
            } else {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    printf("Recv failed: %d\n", error);
                }
            }
            break;
        }
        
        if (header_read != sizeof(struct network_header)) {
            printf("Incomplete header received: %d bytes\n", header_read);
            continue;
        }
        
        // Парсим заголовок
        struct network_header *header = (struct network_header *)recv_buffer;
        uint32_t magic = ntohl_win(header->magic);
        uint32_t type = ntohl_win(header->type);
        uint32_t size = ntohl_win(header->size);
        uint32_t checksum = ntohl_win(header->checksum);
        
        // Проверяем магическое число
        if (magic != NETWORK_MAGIC) {
            printf("Invalid magic: 0x%x\n", magic);
            continue;
        }
        
        printf("\n=== Packet #%d ===\n", ++buffer_count);
        printf("Type: %d, Size: %d bytes\n", type, size);
        
        // Читаем оставшиеся данные
        if (size > 0 && size < sizeof(recv_buffer) - sizeof(struct network_header)) {
            int data_read = recv(sockfd, recv_buffer + sizeof(struct network_header), size, 0);
            if (data_read <= 0) {
                printf("Failed to read packet data: %d\n", WSAGetLastError());
                break;
            }
            
            if (data_read != size) {
                printf("Incomplete data received: expected %d, got %d\n", size, data_read);
                continue;
            }
            
            // Проверяем контрольную сумму
            uint32_t calculated_csum = calculate_checksum(
                recv_buffer + sizeof(struct network_header), size);
            
            if (calculated_csum != checksum) {
                printf("Checksum mismatch: expected %u, got %u\n", checksum, calculated_csum);
                continue;
            }
            
            // Обрабатываем пакет в зависимости от типа
            switch (type) {
                case PACKET_TYPE_BUFFER: {
                    struct buffer buf;
                    deserialize_buffer(recv_buffer, &buf);
                    
                    printf("Buffer Information:\n");
                    printf("  Dimensions: %dx%d pixels\n", buf.width, buf.height);
                    printf("  Type: %s\n", buf.type == 0 ? "SHM" : buf.type == 1 ? "DMA-BUF" : "Unknown");
                    printf("  Format: %s\n", format_to_string(buf.format));
                    printf("  Size: %zu bytes\n", buf.size);
                    
                    // Дополнительная информация в зависимости от типа буфера
                    if (buf.type == 0) { // SHM
                        printf("  Stride: %d bytes\n", buf.shm.stride);
                        printf("  Data offset: %zu bytes\n", buf.shm.data_offset);
                    } else if (buf.type == 1) { // DMA-BUF
                        printf("  Planes: %d\n", buf.dmabuf.num_planes);
                        printf("  Modifier: 0x%llx\n", buf.dmabuf.modifier);
                        for (uint32_t i = 0; i < buf.dmabuf.num_planes; i++) {
                            printf("  Plane %d: stride=%d, offset=%d\n", 
                                   i, buf.dmabuf.strides[i], buf.dmabuf.offsets[i]);
                        }
                    }
                    break;
                }
                case PACKET_TYPE_PING:
                    printf("Ping received\n");
                    // Можно отправить pong обратно
                    break;
                default:
                    printf("Unknown packet type: %d\n", type);
                    break;
            }
        } else {
            printf("Invalid packet size: %d\n", size);
        }
    }
}

int main() {
    WSADATA wsaData;
    int result;
    int connection_attempts = 0;
    
    // Устанавливаем обработчик Ctrl+C
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        printf("Error setting console control handler\n");
        return 1;
    }
    
    // Инициализация Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    
    printf("Client started. Press Ctrl+C to exit.\n");
    
    while (g_running) {
        SOCKET sockfd = connect_to_server();
        
        if (sockfd != INVALID_SOCKET) {
            connection_attempts = 0; // Сброс счетчика при успешном подключении
            run_client(sockfd);
            closesocket(sockfd);
            printf("Connection lost.\n");
        } else {
            connection_attempts++;
            printf("Connection attempt %d failed.\n", connection_attempts);
        }
        
        // Если не выходим по Ctrl+C, пытаемся переподключиться
        if (g_running) {
            printf("Reconnecting in %d seconds...\n", RECONNECT_DELAY / 1000);
            Sleep(RECONNECT_DELAY);
        }
    }
    
    // Очистка
    WSACleanup();
    printf("Client shutdown complete.\n");
    
    // Ждем нажатия любой клавиши перед закрытием
    printf("Press any key to exit...");
    getchar();
    
    return 0;
}