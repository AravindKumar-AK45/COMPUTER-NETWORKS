#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 65432
#define BUFFER_SIZE 2048
#define DB_FILE "students.txt"
#define LOG_FILE "server.log"

// Structure defining the Student schema
typedef struct {
    char reg_num[20];
    char name[50];
    char dept[20];
    int semester;
    int marks[3]; // Supports 3 subjects for simple demonstration
    char grades[3][4];
    float cgpa;
} Student;

// Utility functions
void log_transaction(const char* ip, const char* action, const char* status, const char* msg) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    
    time_t now = time(NULL);
    char* timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline
    
    fprintf(log, "[%s] Client %s | Action: %s | Status: %s | Msg: %s\n", timestamp, ip, action, status, msg);
    fclose(log);
}

void compute_grades_and_cgpa(Student *s) {
    float total_points = 0;
    for (int i = 0; i < 3; i++) {
        int m = s->marks[i];
        int gp = 0;
        if (m >= 90) { gp = 10; strcpy(s->grades[i], "O"); }
        else if (m >= 80) { gp = 9; strcpy(s->grades[i], "A+"); }
        else if (m >= 70) { gp = 8; strcpy(s->grades[i], "A"); }
        else if (m >= 60) { gp = 7; strcpy(s->grades[i], "B+"); }
        else if (m >= 50) { gp = 6; strcpy(s->grades[i], "B"); }
        else { gp = 0; strcpy(s->grades[i], "RA"); }
        total_points += gp;
    }
    s->cgpa = total_points / 3.0;
}

// Core DB Operations
int add_student(Student s, const char* client_ip) {
    FILE *db = fopen(DB_FILE, "r");
    if (db) {
        Student temp;
        while (fread(&temp, sizeof(Student), 1, db)) {
            if (strcmp(temp.reg_num, s.reg_num) == 0) {
                fclose(db);
                log_transaction(client_ip, "ADD", "ERROR", "Duplicate Register Number");
                return 0; // Record exists
            }
        }
        fclose(db);
    }
    
    compute_grades_and_cgpa(&s);
    db = fopen(DB_FILE, "ab");
    if (!db) return -1;
    
    fwrite(&s, sizeof(Student), 1, db);
    fclose(db);
    log_transaction(client_ip, "ADD", "SUCCESS", s.reg_num);
    return 1;
}

int search_student(const char* reg_num, Student *result, const char* client_ip) {
    FILE *db = fopen(DB_FILE, "rb");
    if (!db) return 0;
    
    Student temp;
    while (fread(&temp, sizeof(Student), 1, db)) {
        if (strcmp(temp.reg_num, reg_num) == 0) {
            *result = temp;
            fclose(db);
            log_transaction(client_ip, "SEARCH", "SUCCESS", reg_num);
            return 1;
        }
    }
    fclose(db);
    log_transaction(client_ip, "SEARCH", "ERROR", "Record Not Found");
    return 0;
}

int update_student(Student updated, const char* client_ip) {
    FILE *db = fopen(DB_FILE, "rb+");
    if (!db) return 0;
    
    Student temp;
    long pos;
    int found = 0;
    
    while (fread(&temp, sizeof(Student), 1, db)) {
        if (strcmp(temp.reg_num, updated.reg_num) == 0) {
            pos = ftell(db) - sizeof(Student);
            fseek(db, pos, SEEK_SET);
            compute_grades_and_cgpa(&updated);
            fwrite(&updated, sizeof(Student), 1, db);
            found = 1;
            break;
        }
    }
    fclose(db);
    if (found) {
        log_transaction(client_ip, "UPDATE", "SUCCESS", updated.reg_num);
        return 1;
    }
    log_transaction(client_ip, "UPDATE", "ERROR", "Record Not Found");
    return 0;
}

int delete_student(const char* reg_num, const char* client_ip) {
    FILE *db = fopen(DB_FILE, "rb");
    if (!db) return 0;
    
    FILE *temp_db = fopen("temp.txt", "wb");
    Student temp;
    int found = 0;
    
    while (fread(&temp, sizeof(Student), 1, db)) {
        if (strcmp(temp.reg_num, reg_num) != 0) {
            fwrite(&temp, sizeof(Student), 1, temp_db);
        } else {
            found = 1;
        }
    }
    fclose(db);
    fclose(temp_db);
    
    remove(DB_FILE);
    rename("temp.txt", DB_FILE);
    
    if (found) {
        log_transaction(client_ip, "DELETE", "SUCCESS", reg_num);
        return 1;
    }
    log_transaction(client_ip, "DELETE", "ERROR", "Record Not Found");
    return 0;
}

int get_topper(Student *topper, const char* client_ip) {
    FILE *db = fopen(DB_FILE, "rb");
    if (!db) return 0;
    
    Student temp;
    int found = 0;
    float max_cgpa = -1.0;
    
    while (fread(&temp, sizeof(Student), 1, db)) {
        if (temp.cgpa > max_cgpa) {
            max_cgpa = temp.cgpa;
            *topper = temp;
            found = 1;
        }
    }
    fclose(db);
    if (found) {
        log_transaction(client_ip, "TOPPER", "SUCCESS", topper->reg_num);
        return 1;
    }
    return 0;
}

// Request processing pipeline
void process_client(int client_sock, const char* client_ip) {
    char buffer[BUFFER_SIZE];
    int read_size;
    
    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        char response[BUFFER_SIZE] = {0};
        char action[10];
        
        sscanf(buffer, "%s", action);
        
        if (strcmp(action, "ADD") == 0) {
            Student s;
            sscanf(buffer, "ADD %s %s %s %d %d %d %d", s.reg_num, s.name, s.dept, &s.semester, &s.marks[0], &s.marks[1], &s.marks[2]);
            int res = add_student(s, client_ip);
            if (res == 1) strcpy(response, "SUCCESS: Record added efficiently.");
            else if (res == 0) strcpy(response, "ERROR: Register number already exists.");
            else strcpy(response, "ERROR: Database File Write Issue.");
            
        } else if (strcmp(action, "SEARCH") == 0) {
            char reg[20];
            sscanf(buffer, "SEARCH %s", reg);
            Student s;
            if (search_student(reg, &s, client_ip)) {
                sprintf(response, "SUCCESS %s %s %s %d %.2f", s.reg_num, s.name, s.dept, s.semester, s.cgpa);
            } else {
                strcpy(response, "ERROR: Student Not Found.");
            }
            
        } else if (strcmp(action, "UPDATE") == 0) {
            Student s;
            sscanf(buffer, "UPDATE %s %s %s %d %d %d %d", s.reg_num, s.name, s.dept, &s.semester, &s.marks[0], &s.marks[1], &s.marks[2]);
            if (update_student(s, client_ip)) {
                strcpy(response, "SUCCESS: Record updated successfully.");
            } else {
                strcpy(response, "ERROR: Update Target Match Failed.");
            }
            
        } else if (strcmp(action, "DELETE") == 0) {
            char reg[20];
            sscanf(buffer, "DELETE %s", reg);
            if (delete_student(reg, client_ip)) {
                strcpy(response, "SUCCESS: Record deleted safely.");
            } else {
                strcpy(response, "ERROR: Deletion Target Match Failed.");
            }
            
        } else if (strcmp(action, "DISPLAY") == 0) {
            FILE *db = fopen(DB_FILE, "rb");
            if (!db) {
                strcpy(response, "EMPTY");
            } else {
                Student s;
                char segment[256];
                strcpy(response, "DATA\n");
                while (fread(&s, sizeof(Student), 1, db)) {
                    sprintf(segment, "Reg: %s | Name: %s | Dept: %s | Sem: %d | CGPA: %.2f\n", s.reg_num, s.name, s.dept, s.semester, s.cgpa);
                    strcat(response, segment);
                }
                fclose(db);
                log_transaction(client_ip, "DISPLAY", "SUCCESS", "All Records Dumped");
            }
            
        } else if (strcmp(action, "TOPPER") == 0) {
            Student s;
            if (get_topper(&s, client_ip)) {
                sprintf(response, "SUCCESS Topper Is: %s | Name: %s | Dept: %s | CGPA: %.2f", s.reg_num, s.name, s.dept, s.cgpa);
            } else {
                strcpy(response, "ERROR: No student database entries discovered.");
            }
        } else {
            strcpy(response, "ERROR: Unknown request protocol command issued.");
        }
        
        send(client_sock, response, strlen(response), 0);
    }
    close(client_sock);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    server_fd = socket(AF_INET, socket.SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }
    
    listen(server_fd, 5);
    printf("[SERVER STARTED] Listening on port %d...\n", PORT);
    
    while ((client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size))) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[CONNECTION ACCEPED] Client IP: %s\n", client_ip);
        
        // Processing single synchronous wrapper lifecycle
        process_client(client_sock, client_ip);
    }
    
    close(server_fd);
    return 0;
}

