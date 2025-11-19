#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(12345);

    // Подключение к серверу
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server:\n");

    // Обмен сообщениями
    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        
        // Отправка сообщения
        send(client_socket, buffer, strlen(buffer), 0);
        
        // Получение ответа
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Сервер отключился\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        printf("Ответ: %s", buffer);
        
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}