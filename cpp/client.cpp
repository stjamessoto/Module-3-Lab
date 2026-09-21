/*
 * client.cpp
 * TCP client in C++.
 *
 * Usage:
 *   ./client_cpp                 sends the default message "Hello C++ Server"
 *   ./client_cpp "some text"     sends your own message
 *   ./client_cpp shutdown        asks the server to shut down
 *   ./client_cpp --big 10000     sends 10000 characters (large message test)
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(int argc, char *argv[]) {
    std::string message;

    if (argc > 1 && std::string(argv[1]) == "--big") {
        size_t size = (argc > 2) ? static_cast<size_t>(std::atoi(argv[2])) : 10000;
        message = std::string(size, 'A');
    } else if (argc > 1) {
        message = argv[1];
    } else {
        message = "Hello C++ Server";
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    send(sock, message.c_str(), message.size(), 0);

    char response[1024];
    ssize_t n = read(sock, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        std::cout << "Server response: " << response << std::endl;
    }

    close(sock);
    return 0;
}
