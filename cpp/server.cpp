/*
 * server.cpp
 * Fork based TCP server in C++ (Exercise 2, final version).
 *
 * Same design as c/server.c: SIGCHLD reaping, SO_REUSEADDR, chunked
 * reading with std::string, and a clean "shutdown" command.
 */

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define CHUNK_SIZE 1024

static int server_sock = -1;

// Reap every finished child so none stay behind as zombies.
void sigchld_handler(int) {
    int saved_errno = errno;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }
    errno = saved_errno;
}

// Close the listening socket and exit cleanly.
void shutdown_handler(int) {
    std::cout << "\nShutting down: closing listening socket and exiting." << std::endl;
    if (server_sock >= 0) {
        close(server_sock);
    }
    std::exit(0);
}

// Read a message of any size by appending chunks to a std::string.
std::string read_full_message(int client_sock) {
    std::string message;
    char chunk[CHUNK_SIZE];
    while (true) {
        ssize_t n = read(client_sock, chunk, CHUNK_SIZE);
        if (n <= 0) {
            break;
        }
        message.append(chunk, static_cast<size_t>(n));
        if (n < CHUNK_SIZE) {
            break; // short read means the client has finished sending
        }
    }
    return message;
}

void handle_client(int client_sock) {
    std::string message = read_full_message(client_sock);
    std::cout << "Received " << message.size() << " bytes: "
              << message.substr(0, 60) << (message.size() > 60 ? "..." : "")
              << std::endl;

    if (message == "shutdown") {
        write(client_sock, "Server is shutting down", 23);
        close(client_sock);
        kill(getppid(), SIGTERM); // ask the parent server to exit cleanly
        return;
    }

    write(client_sock, "Hello from C++ server", 21);
    close(client_sock);
}

int main() {
    int client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int reuse = 1;

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, nullptr);

    std::signal(SIGTERM, shutdown_handler);
    std::signal(SIGINT, shutdown_handler);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "C++ server listening on port " << PORT
              << " (PID " << getpid() << ")" << std::endl;

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) {
            if (errno != EINTR) {
                perror("accept");
            }
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_sock);
            handle_client(client_sock);
            std::exit(0);
        } else if (pid > 0) {
            close(client_sock);
        } else {
            perror("fork");
            close(client_sock);
        }
    }
    return 0;
}
