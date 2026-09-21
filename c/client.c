/*
 * client.c
 * TCP client in C.
 *
 * Usage:
 *   ./client                 sends the default message "Hello Server"
 *   ./client "some text"     sends your own message
 *   ./client shutdown        asks the server to shut down (Q5)
 *   ./client --big 10000     sends 10000 characters (Q4 large message test)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char response[1024];
    char *message;
    size_t message_len;
    int allocated = 0;

    if (argc > 1 && strcmp(argv[1], "--big") == 0) {
        size_t size = (argc > 2) ? (size_t)atoi(argv[2]) : 10000;
        message = malloc(size + 1);
        if (!message) {
            perror("malloc");
            return 1;
        }
        memset(message, 'A', size);
        message[size] = '\0';
        message_len = size;
        allocated = 1;
    } else if (argc > 1) {
        message = argv[1];
        message_len = strlen(message);
    } else {
        message = "Hello Server";
        message_len = strlen(message);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    send(sock, message, message_len, 0);

    ssize_t n = read(sock, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        printf("Server response: %s\n", response);
    }

    close(sock);
    if (allocated) {
        free(message);
    }
    return 0;
}
