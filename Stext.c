#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8082
#define BUFFER_SIZE 1024

void handle_smain(int smain_socket);

int main() {
    int server_socket, smain_socket;
    struct sockaddr_in server_addr, smain_addr;
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

    printf("Stext server listening on port %d\n", PORT);

    while (1) {
        addr_size = sizeof(smain_addr);
        smain_socket = accept(server_socket, (struct sockaddr*)&smain_addr, &addr_size);
        if (smain_socket < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            close(server_socket);
            handle_smain(smain_socket);
            close(smain_socket);
            exit(0);
        } else {
            close(smain_socket);
        }
    }

    close(server_socket);
    return 0;
}

void handle_smain(int smain_socket) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
    int file_size, bytes_received, bytes_written;
    int file_fd;

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        read(smain_socket, buffer, BUFFER_SIZE);
        sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "upload") == 0) {
            read(smain_socket, &file_size, sizeof(file_size));
            file_fd = open(arg1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (file_fd < 0) {
                perror("File open failed");
                continue;
            }
            while (file_size > 0) {
                bytes_received = read(smain_socket, buffer, BUFFER_SIZE);
                bytes_written = write(file_fd, buffer, bytes_received);
                file_size -= bytes_written;
            }
            close(file_fd);
        } else if (strcmp(command, "download") == 0) {
            file_fd = open(arg1, O_RDONLY);
            if (file_fd < 0) {
                perror("File open failed");
                continue;
            }
            file_size = lseek(file_fd, 0, SEEK_END);
            lseek(file_fd, 0, SEEK_SET);
            write(smain_socket, &file_size, sizeof(file_size));
            while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                write(smain_socket, buffer, bytes_received);
            }
            close(file_fd);
        } else if (strcmp(command, "delete") == 0) {
            if (unlink(arg1) < 0) {
                perror("File deletion failed");
            }
        } else {
            write(smain_socket, "Invalid command\n", strlen("Invalid command\n"));
        }
    }
}
