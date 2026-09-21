/*
 * dos_sim_local.c
 * Exercise 4: safe DoS simulation, localhost only.
 *
 *  Only ever connects to 127.0.0.1 (hard coded, not configurable).
 *  Opens up to max_connections TCP connections and sends no data.
 *  Holds every connection open for hold_seconds, then closes them all.
 *
 * Use only against your own server on your own machine.
 *
 * Usage: ./dos_sim_local <port> <max_connections> <hold_seconds>
 * Example: ./dos_sim_local 8080 200 30
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port> <max_connections> <hold_seconds>\n", argv[0]);
        fprintf(stderr, "Example: %s 8080 200 30\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_conn = atoi(argv[2]);
    int hold_seconds = atoi(argv[3]);

    if (max_conn <= 0 || max_conn > 10000) {
        fprintf(stderr, "max_connections must be between 1 and 10000\n");
        return 1;
    }
    if (hold_seconds <= 0) {
        fprintf(stderr, "hold_seconds must be greater than 0\n");
        return 1;
    }

    const char *target_ip = "127.0.0.1"; /* enforced loopback */
    printf("Starting local only connection storm to %s:%d\n", target_ip, port);

    int *socks = calloc(max_conn, sizeof(int));
    if (!socks) {
        perror("calloc");
        return 1;
    }

    int opened = 0;
    for (int i = 0; i < max_conn; ++i) {
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) {
            perror("socket");
            socks[i] = -1;
            continue;
        }

        struct sockaddr_in srv;
        memset(&srv, 0, sizeof(srv));
        srv.sin_family = AF_INET;
        srv.sin_port = htons(port);
        inet_pton(AF_INET, target_ip, &srv.sin_addr);

        if (connect(s, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
            fprintf(stderr, "[%d] connect failed: %s\n", i, strerror(errno));
            close(s);
            socks[i] = -1;
            usleep(1000);
            continue;
        }

        socks[i] = s;
        opened++;
        if ((i + 1) % 50 == 0 || i == max_conn - 1) {
            printf("Opened %d/%d connections\n", opened, max_conn);
        }
        usleep(1000); /* small pacing */
    }

    printf("Holding %d connections for %d seconds (local test only).\n", opened, hold_seconds);
    sleep(hold_seconds);

    for (int i = 0; i < max_conn; ++i) {
        if (socks[i] >= 0) {
            close(socks[i]);
        }
    }
    free(socks);
    printf("Done. Closed all sockets.\n");
    return 0;
}
