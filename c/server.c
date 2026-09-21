/*
 * server.c
 * Fork based TCP server in C (final version).
 *
 * Fixes added for the Exercise 3 challenge questions:
 *   Q1 Zombie processes : a SIGCHLD handler reaps children with waitpid().
 *   Q3 Port reuse       : SO_REUSEADDR lets the server restart immediately.
 *   Q4 Large messages   : the message is read in chunks into a growing buffer.
 *   Q5 Graceful shutdown: the message "shutdown" closes the server cleanly.
 *
 * Note: printf and exit are used inside signal handlers to keep the lab code
 * short. Production code should only call async signal safe functions there.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define CHUNK_SIZE 1024

static int server_sock = -1;

/* Q1: reap every finished child so none stay behind as zombies. */
void sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* keep reaping until no finished child is left */
    }
    errno = saved_errno;
}

/* Q5: close the listening socket and exit cleanly. */
void shutdown_handler(int sig) {
    (void)sig;
    printf("\nShutting down: closing listening socket and exiting.\n");
    if (server_sock >= 0) {
        close(server_sock);
    }
    exit(0);
}

/* Q4: read a message of any size without overflowing a fixed buffer.
 * Returns a malloc'd, NUL terminated string that the caller must free. */
char *read_full_message(int client_sock) {
    size_t capacity = CHUNK_SIZE * 2;
    size_t used = 0;
    char *message = malloc(capacity);
    if (!message) {
        return NULL;
    }

    while (1) {
        if (used + CHUNK_SIZE + 1 > capacity) {
            capacity *= 2;
            char *bigger = realloc(message, capacity);
            if (!bigger) {
                free(message);
                return NULL;
            }
            message = bigger;
        }
        ssize_t n = read(client_sock, message + used, CHUNK_SIZE);
        if (n <= 0) {
            break;
        }
        used += (size_t)n;
        if (n < CHUNK_SIZE) {
            break; /* short read means the client has finished sending */
        }
    }
    message[used] = '\0';
    return message;
}

void handle_client(int client_sock) {
    char *message = read_full_message(client_sock);
    if (!message) {
        close(client_sock);
        return;
    }

    size_t length = strlen(message);
    printf("Received %zu bytes: %.60s%s\n", length, message,
           length > 60 ? "..." : "");

    if (strcmp(message, "shutdown") == 0) {
        write(client_sock, "Server is shutting down", 23);
        close(client_sock);
        free(message);
        kill(getppid(), SIGTERM); /* ask the parent server to exit cleanly */
        return;
    }

    write(client_sock, "Hello from server", 17);
    close(client_sock);
    free(message);
}

int main() {
    int client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int reuse = 1;

    /* Q1: install the SIGCHLD handler. SA_RESTART keeps accept() from failing. */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    /* Q5: SIGTERM (from a shutdown command) and Ctrl+C both exit cleanly. */
    signal(SIGTERM, shutdown_handler);
    signal(SIGINT, shutdown_handler);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    /* Q3: allow the port to be reused right after the server stops. */
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d (PID %d)\n", PORT, getpid());
    fflush(stdout); /* stop children from inheriting and re-printing buffered output */

    while (1) {
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
            close(server_sock); /* the child does not need the listening socket */
            handle_client(client_sock);
            exit(0);
        } else if (pid > 0) {
            close(client_sock); /* the parent does not need the client socket */
        } else {
            perror("fork");
            close(client_sock);
        }
    }
    return 0;
}
