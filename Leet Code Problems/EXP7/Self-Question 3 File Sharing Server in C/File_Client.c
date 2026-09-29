#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>

#define PORT 8081
#define SHARE_DIR "shared_files"
#define MAX_LINE 1024
#define BUF_SIZE 4096

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void log_event(const char *fmt, ...) {
    pthread_mutex_lock(&log_mutex);

    FILE *fp = fopen("file_server.log", "a");
    if (fp) {
        time_t now = time(NULL);
        char tbuf[64];

        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(fp, "[%s] ", tbuf);

        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);

        fprintf(fp, "\n");
        fclose(fp);
    }

    pthread_mutex_unlock(&log_mutex);
}

int valid_filename(const char *name) {
    if (name == NULL || strlen(name) == 0) return 0;
    if (strstr(name, "..")) return 0;

    for (int i = 0; name[i]; i++) {
        if (name[i] == '/' || name[i] == '\\') return 0;
    }

    return 1;
}

void list_files(int client) {
    pthread_mutex_lock(&file_mutex);

    DIR *dir = opendir(SHARE_DIR);
    if (!dir) {
        pthread_mutex_unlock(&file_mutex);
        send_line(client, "ERR Cannot open shared_files directory");
        return;
    }

    send_line(client, "OK");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char path[512];
        struct stat st;

        snprintf(path, sizeof(path), "%s/%s", SHARE_DIR, entry->d_name);

        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            send_line(client, entry->d_name);
        }
    }

    closedir(dir);
    send_line(client, "END");

    pthread_mutex_unlock(&file_mutex);
}

void send_file(int client, const char *filename) {
    if (!valid_filename(filename)) {
        send_line(client, "ERR Invalid filename");
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", SHARE_DIR, filename);

    pthread_mutex_lock(&file_mutex);

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        pthread_mutex_unlock(&file_mutex);
        send_line(client, "ERR File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char header[100];
    snprintf(header, sizeof(header), "OK %ld", size);
    send_line(client, header);

    char buffer[BUF_SIZE];
    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send_all(client, buffer, n) < 0) break;
    }

    fclose(fp);
    pthread_mutex_unlock(&file_mutex);
}

void *handle_client(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char line[MAX_LINE];

    send_line(client, "Connected to File Sharing Server");
    send_line(client, "Commands: LIST, GET filename, QUIT");

    log_event("Client connected");

    while (1) {
        ssize_t n = recv_line(client, line, sizeof(line));

        if (n <= 0) break;

        if (strcasecmp(line, "LIST") == 0) {
            log_event("LIST command received");
            list_files(client);
        }
        else if (strncasecmp(line, "GET ", 4) == 0) {
            char *filename = line + 4;
            while (*filename == ' ') filename++;

            log_event("GET command received for file: %s", filename);
            send_file(client, filename);
        }
        else if (strcasecmp(line, "QUIT") == 0) {
            send_line(client, "BYE");
            break;
        }
        else {
            send_line(client, "ERR Invalid command");
        }
    }

    close(client);
    log_event("Client disconnected");
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    mkdir(SHARE_DIR, 0755);

    int server = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 10);

    printf("File Sharing Server running on port %d...\n", PORT);

    while (1) {
        int client = accept(server, NULL, NULL);

        int *pclient = malloc(sizeof(int));
        *pclient = client;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server);
    return 0;
}