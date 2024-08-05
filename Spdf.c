#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8081
#define BUFFER_SIZE 1024

void handle_client(int client_socket);
void handle_pdf_request(int client_socket, char *filename);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Spdf server listening on port %d\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            close(server_socket);
            handle_client(client_socket);
            close(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE], arg1[BUFFER_SIZE];

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        read(client_socket, buffer, BUFFER_SIZE);
        sscanf(buffer, "%s %s", command, arg1);

        if (strcmp(command, "dfile") == 0) {
            handle_pdf_request(client_socket, arg1);
        } else {
            write(client_socket, "Invalid command\n", strlen("Invalid command\n"));
        }
    }
}

void handle_pdf_request(int client_socket, char *filename) {
    char buffer[BUFFER_SIZE];
    int file_size, bytes_read, bytes_sent;
    int src_fd;

    src_fd = open(filename, O_RDONLY);
    if (src_fd < 0) {
        perror("File open failed");
        write(client_socket, "File not found\n", strlen("File not found\n"));
        return;
    }

    file_size = lseek(src_fd, 0, SEEK_END);
    lseek(src_fd, 0, SEEK_SET);
    write(client_socket, &file_size, sizeof(file_size));

    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_sent = write(client_socket, buffer, bytes_read);
    }

    close(src_fd);
}
