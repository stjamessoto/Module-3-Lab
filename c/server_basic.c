/*
 * server_basic.c
 * Exercise 1: the basic fork based server from the lab starter code.
 *
 * This version intentionally has NO zombie handling, NO SO_REUSEADDR and
 * reads only one 1024 byte chunk. It exists so you can demonstrate the
 * problems described in Exercise 3. The fixed version is server.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void handle_client(int client_sock) {
    char buffer[1024];
    read(client_sock, buffer, sizeof(buffer));
    printf("Received: %s\n", buffer);
    write(client_sock, "Hello from server", 17);
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Basic server (no zombie handling) listening on port %d\n", PORT);
    fflush(stdout);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (fork() == 0) {
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);
    }
    return 0;
}
