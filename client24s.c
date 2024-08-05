#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void upload_file(int socket, const char *filename, const char *destination_path);
void download_file(int socket, const char *filename);
void remove_file(int socket, const char *filename);
void create_tar(int socket, const char *filetype);
void display_files(int socket, const char *pathname);
void list_files(int socket, const char *directory);

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <command> <args...>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "ufile") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s ufile <filename> <destination_path>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        upload_file(client_socket, argv[2], argv[3]);
    } else if (strcmp(argv[1], "dfile") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s dfile <filename>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        download_file(client_socket, argv[2]);
    } else if (strcmp(argv[1], "rmfile") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s rmfile <filename>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        remove_file(client_socket, argv[2]);
    } else if (strcmp(argv[1], "dtar") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s dtar <filetype>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        create_tar(client_socket, argv[2]);
    } else if (strcmp(argv[1], "display") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s display <pathname>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        display_files(client_socket, argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s list <directory>\n", argv[0]);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        list_files(client_socket, argv[2]);
    } else {
        fprintf(stderr, "Invalid command\n");
    }

    close(client_socket);
    return 0;
}

void upload_file(int socket, const char *filename, const char *destination_path) {
    char buffer[BUFFER_SIZE];
    int file_fd;
    ssize_t bytes_read, bytes_sent;
    off_t file_size, total_bytes_sent = 0;

    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("File open failed");
        return;
    }

    // Send command
    snprintf(buffer, sizeof(buffer), "ufile %s %s", filename, destination_path);
    write(socket, buffer, strlen(buffer));

    // Send file size
    file_size = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET);
    write(socket, &file_size, sizeof(file_size));

    // Send file content
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_sent = write(socket, buffer, bytes_read);
        if (bytes_sent < 0) {
            perror("Error sending file");
            close(file_fd);
            return;
        }
        total_bytes_sent += bytes_sent;
        printf("Uploaded %ld/%ld bytes (%.2f%%)\n", total_bytes_sent, file_size, (total_bytes_sent / (float)file_size) * 100);
    }

    close(file_fd);

    bzero(buffer, sizeof(buffer));
    read(socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void download_file(int socket, const char *filename) {
    char buffer[BUFFER_SIZE];
    int file_fd;
    ssize_t bytes_read, bytes_written;
    off_t file_size;

    // Send command
    snprintf(buffer, sizeof(buffer), "dfile %s", filename);
    write(socket, buffer, strlen(buffer));

    // Receive file size
    read(socket, &file_size, sizeof(file_size));

    file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) {
        perror("File open failed");
        return;
    }

    // Receive file content
    while (file_size > 0) {
        bytes_read = read(socket, buffer, BUFFER_SIZE);
        bytes_written = write(file_fd, buffer, bytes_read);
        file_size -= bytes_written;
    }

    close(file_fd);

    bzero(buffer, sizeof(buffer));
    read(socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void remove_file(int socket, const char *filename) {
    char buffer[BUFFER_SIZE];

    // Send command
    snprintf(buffer, sizeof(buffer), "rmfile %s", filename);
    write(socket, buffer, strlen(buffer));

    bzero(buffer, sizeof(buffer));
    read(socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void create_tar(int socket, const char *filetype) {
    char buffer[BUFFER_SIZE];

    // Send command
    snprintf(buffer, sizeof(buffer), "dtar %s", filetype);
    write(socket, buffer, strlen(buffer));

    bzero(buffer, sizeof(buffer));
    read(socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void display_files(int socket, const char *pathname) {
    char buffer[BUFFER_SIZE];

    // Send command
    snprintf(buffer, sizeof(buffer), "display %s", pathname);
    write(socket, buffer, strlen(buffer));

    while (read(socket, buffer, sizeof(buffer)) > 0) {
        printf("%s", buffer);
    }
}

void list_files(int socket, const char *directory) {
    char buffer[BUFFER_SIZE];

    // Send command
    snprintf(buffer, sizeof(buffer), "list %s", directory);
    write(socket, buffer, strlen(buffer));

    while (read(socket, buffer, sizeof(buffer)) > 0) {
        printf("%s", buffer);
    }
}
