#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 65432
#define BUFFER_SIZE 2048

void clean_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char payload[BUFFER_SIZE];
    int choice;

    sock = socket(AF_INET, socket.SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Connection failed");
        return 1;
    }
    printf("[⚡] Connected securely to the C Student Result Management Server.\n");

    while (1) {
        printf("\n=== STUDENT RESULT MANAGEMENT SYSTEM (C CLI) ===\n");
        printf("1. Add Student Record\n");
        printf("2. Search Student Record\n");
        printf("3. Update Student Record\n");
        printf("4. Delete Student Record\n");
        printf("5. Display All Records\n");
        printf("6. Generate Class Topper\n");
        printf("7. Exit\n");
        printf("Enter Choice (1-7): ");
        scanf("%d", &choice);
        clean_stdin();

        if (choice == 7) {
            printf("Exiting Client application.\n");
            break;
        }

        memset(payload, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);

        char reg[20], name[50], dept[20];
        int sem, m1, m2, m3;

        switch (choice) {
            case 1: // ADD
                printf("Register Number: "); scanf("%s", reg);
                printf("Name: "); scanf("%s", name);
                printf("Department: "); scanf("%s", dept);
                printf("Semester: "); scanf("%d", &sem);
                printf("Enter Marks for Subject 1, 2, 3: "); scanf("%d %d %d", &m1, &m2, &m3);
                sprintf(payload, "ADD %s %s %s %d %d %d %d", reg, name, dept, sem, m1, m2, m3);
                break;
            case 2: // SEARCH
                printf("Enter Register Number: "); scanf("%s", reg);
                sprintf(payload, "SEARCH %s", reg);
                break;
            case 3: // UPDATE
                printf("Enter Target Register Number: "); scanf("%s", reg);
                printf("Enter New Name: "); scanf("%s", name);
                printf("Enter New Department: "); scanf("%s", dept);
                printf("Enter New Semester: "); scanf("%d", &sem);
                printf("Enter New Marks for Subject 1, 2, 3: "); scanf("%d %d %d", &m1, &m2, &m3);
                sprintf(payload, "UPDATE %s %s %s %d %d %d %d", reg, name, dept, sem, m1, m2, m3);
                break;
            case 4: // DELETE
                printf("Enter Target Register Number: "); scanf("%s", reg);
                sprintf(payload, "DELETE %s", reg);
                break;
            case 5: // DISPLAY ALL
                strcpy(payload, "DISPLAY");
                break;
            case 6: // TOPPER
                strcpy(payload, "TOPPER");
                break;
            default:
                printf("⚠️ Invalid selection!\n");
                continue;
        }

        // Send down payload buffer pipe
        send(sock, payload, strlen(payload), 0);
        
        // Receive output buffer pipe
        int recv_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        buffer[recv_bytes] = '\0';
        
        printf("\n📢 [SERVER RESPONSE]:\n%s\n", buffer);
    }

    close(sock);
    return 0;
}

