#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8082
#define MAX_LINE 1024

int send_all(int sock, const void *buf, size_t len) {
    const char *p = buf;

    while (len > 0) {
        ssize_t n = send(sock, p, len, 0);
        if (n <= 0) return -1;

        p += n;
        len -= n;
    }

    return 0;
}

int send_line(int sock, const char *msg) {
    if (send_all(sock, msg, strlen(msg)) < 0) return -1;
    return send_all(sock, "\n", 1);
}

ssize_t recv_line(int sock, char *buf, size_t max) {
    size_t i = 0;
    char c;

    while (i < max - 1) {
        ssize_t n = recv(sock, &c, 1, 0);

        if (n <= 0) {
            if (i == 0) return n;
            break;
        }

        if (c == '\r') continue;
        if (c == '\n') break;

        buf[i++] = c;
    }

    buf[i] = '\0';
    return i;
}

void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char line[MAX_LINE];

    while (1) {
        ssize_t n = recv_line(sock, line, sizeof(line));

        if (n <= 0) {
            printf("\nDisconnected from server.\n");
            exit(0);
        }

        printf("\n%s\n", line);
        printf("> ");
        fflush(stdout);
    }

    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        return 1;
    }

    char line[MAX_LINE];
    char name[100];

    recv_line(sock, line, sizeof(line));
    printf("%s ", line);

    fgets(name, sizeof(name), stdin);
    trim_newline(name);

    send_line(sock, name);

    recv_line(sock, line, sizeof(line));
    printf("%s\n", line);

    pthread_t tid;
    pthread_create(&tid, NULL, receive_messages, &sock);

    while (1) {
        char msg[MAX_LINE];

        printf("> ");
        fgets(msg, sizeof(msg), stdin);
        trim_newline(msg);

        send_line(sock, msg);

        if (strcasecmp(msg, "/quit") == 0) {
            break;
        }
    }

    close(sock);
    return 0;
}