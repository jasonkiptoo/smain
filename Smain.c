#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket);
void handle_ufile(int client_socket, char *filename, char *destination_path);
void handle_dfile(int client_socket, char *filename);
void handle_rmfile(int client_socket, char *filename);
void handle_dtar(int client_socket, char *filetype);
void handle_display(int client_socket, char *pathname);
void handle_list(int client_socket, char *directory);

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

    printf("Smain server listening on port %d\n", PORT);

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
    char command[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        read(client_socket, buffer, BUFFER_SIZE);
        sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "ufile") == 0) {
            handle_ufile(client_socket, arg1, arg2);
        } else if (strcmp(command, "dfile") == 0) {
            handle_dfile(client_socket, arg1);
        } else if (strcmp(command, "rmfile") == 0) {
            handle_rmfile(client_socket, arg1);
        } else if (strcmp(command, "dtar") == 0) {
            handle_dtar(client_socket, arg1);
        } else if (strcmp(command, "display") == 0) {
            handle_display(client_socket, arg1);
        } else if (strcmp(command, "list") == 0) {
            handle_list(client_socket, arg1);
        } else {
            write(client_socket, "Invalid command\n", strlen("Invalid command\n"));
        }
    }
}

void handle_ufile(int client_socket, char *filename, char *destination_path) {
    char buffer[BUFFER_SIZE];
    int file_size, bytes_received, bytes_written;
    int dest_fd;

    read(client_socket, &file_size, sizeof(file_size));

    if (strstr(filename, ".c")) {
        dest_fd = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (strstr(filename, ".pdf")) {
        // Forward to Spdf server (not implemented here)
        write(client_socket, "Forwarding to Spdf server\n", strlen("Forwarding to Spdf server\n"));
        return;
    } else if (strstr(filename, ".txt")) {
        // Forward to Stext server (not implemented here)
        write(client_socket, "Forwarding to Stext server\n", strlen("Forwarding to Stext server\n"));
        return;
    } else {
        write(client_socket, "Invalid file type\n", strlen("Invalid file type\n"));
        return;
    }

    if (dest_fd < 0) {
        perror("File open failed");
        return;
    }

    while (file_size > 0) {
        bytes_received = read(client_socket, buffer, BUFFER_SIZE);
        bytes_written = write(dest_fd, buffer, bytes_received);
        file_size -= bytes_written;
    }

    close(dest_fd);
    write(client_socket, "File uploaded successfully\n", strlen("File uploaded successfully\n"));
}

void handle_dfile(int client_socket, char *filename) {
    char buffer[BUFFER_SIZE];
    int file_size, bytes_read, bytes_sent;
    int src_fd;

    if (strstr(filename, ".c")) {
        src_fd = open(filename, O_RDONLY);
    } else if (strstr(filename, ".pdf")) {
        // Request from Spdf server (not implemented here)
        write(client_socket, "Requesting from Spdf server\n", strlen("Requesting from Spdf server\n"));
        return;
    } else if (strstr(filename, ".txt")) {
        // Request from Stext server (not implemented here)
        write(client_socket, "Requesting from Stext server\n", strlen("Requesting from Stext server\n"));
        return;
    } else {
        write(client_socket, "Invalid file type\n", strlen("Invalid file type\n"));
        return;
    }

    if (src_fd < 0) {
        perror("File open failed");
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

void handle_rmfile(int client_socket, char *filename) {
    if (strstr(filename, ".c")) {
        if (unlink(filename) < 0) {
            perror("File deletion failed");
        } else {
            write(client_socket, "File deleted successfully\n", strlen("File deleted successfully\n"));
        }
    } else if (strstr(filename, ".pdf")) {
        // Request deletion from Spdf server (not implemented here)
        write(client_socket, "Requesting deletion from Spdf server\n", strlen("Requesting deletion from Spdf server\n"));
    } else if (strstr(filename, ".txt")) {
        // Request deletion from Stext server (not implemented here)
        write(client_socket, "Requesting deletion from Stext server\n", strlen("Requesting deletion from Stext server\n"));
    } else {
        write(client_socket, "Invalid file type\n", strlen("Invalid file type\n"));
    }
}

void handle_dtar(int client_socket, char *filetype) {
    if (strcmp(filetype, ".c") == 0) {
        // Create tar of .c files and send to client (not implemented here)
        write(client_socket, "Creating tar of .c files\n", strlen("Creating tar of .c files\n"));
    } else if (strcmp(filetype, ".pdf") == 0) {
        // Request tar of .pdf files from Spdf server (not implemented here)
        write(client_socket, "Requesting tar of .pdf files from Spdf server\n", strlen("Requesting tar of .pdf files from Spdf server\n"));
    } else if (strcmp(filetype, ".txt") == 0) {
        // Request tar of .txt files from Stext server (not implemented here)
        write(client_socket, "Requesting tar of .txt files from Stext server\n", strlen("Requesting tar of .txt files from Stext server\n"));
    } else {
        write(client_socket, "Invalid file type\n", strlen("Invalid file type\n"));
    }
}

void handle_display(int client_socket, char *pathname) {
    // Combine list of .c, .pdf, and .txt files and send to client (not implemented here)
    write(client_socket, "Displaying files\n", strlen("Displaying files\n"));
}

void handle_list(int client_socket, char *directory) {
    char buffer[BUFFER_SIZE];
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    dir = opendir(directory);
    if (dir == NULL) {
        perror("Failed to open directory");
        write(client_socket, "Error opening directory\n", strlen("Error opening directory\n"));
        return;
    }

    // Send directory listing to client
    while ((entry = readdir(dir)) != NULL) {
        snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
        write(client_socket, buffer, strlen(buffer));
    }

    closedir(dir);
    write(client_socket, "End of directory listing\n", strlen("End of directory listing\n"));
}
