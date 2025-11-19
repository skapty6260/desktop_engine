#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    char buffer[1024];
    int bytes_received;

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    // Привязка сокета
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Прослушивание порта
    if (listen(server_socket, 1) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Socket running on port 12345...\n");

    // Принятие подключения
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Client connected!\n");

    // Прием и отправка данных
    while (1) {
        // Получение данных от клиента
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Received: %s", buffer);

        // Эхо-ответ
        send(client_socket, buffer, bytes_received, 0);
        
        // Если клиент отправил "exit", завершаем соединение
        if (strncmp(buffer, "exit", 4) == 0) {
            printf("Exiting connection...\n");
            break;
        }
    }

    // Закрытие сокетов и очистка
    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}