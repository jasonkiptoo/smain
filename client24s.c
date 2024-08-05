#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_command(int server_socket, char *command);

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("client24s$ ");
        bzero(command, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; // Remove the trailing newline
        handle_command(server_socket, command);
    }

    close(server_socket);
    return 0;
}

void handle_command(int server_socket, char *command) {
    char buffer[BUFFER_SIZE];
    char cmd[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
    int file_fd, file_size, bytes_read;

    sscanf(command, "%s %s %s", cmd, arg1, arg2);

    if (strcmp(cmd, "ufile") == 0) {
        file_fd = open(arg1, O_RDONLY);
        if (file_fd < 0) {
            perror("File open failed");
            return;
        }

        file_size = lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);

        write(server_socket, command, strlen(command));
        write(server_socket, &file_size, sizeof(file_size));

        while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            write(server_socket, buffer, bytes_read);
        }

        close(file_fd);
    } else {
        write(server_socket, command, strlen(command));
        bzero(buffer, BUFFER_SIZE);
        read(server_socket, buffer, BUFFER_SIZE);
        printf("%s\n", buffer);
    }
}
