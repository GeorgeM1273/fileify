#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 5000
#define BUFFER_SIZE 4096 // Increased buffer size for efficient binary file transfer

typedef struct {
    char songName[50];
    char filePath[100];
} Song;

int findFile(char *filename, Song songList[], int size) {
    for (int i = 0; i < size; i++) {
        if (strcmp(songList[i].songName, filename) == 0) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    Song songList[10] = {0};

    strcpy(songList[0].songName, "The");
    strcpy(songList[0].filePath, "The.flac");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    // Allow quick port reuse after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("File Server - PID: %d\n", getpid());
    printf("Waiting for a client on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        printf("Client connected.\n");

        int n = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (n <= 0) {
            close(client_fd);
            continue;
        }

        buffer[n] = '\0';
        // Strip trailing \r and \n characters from client request
        buffer[strcspn(buffer, "\r\n")] = 0;
        printf("Received request for: '%s'\n", buffer);

        int fileIndex = findFile(buffer, songList, 10);

        if (fileIndex == -1) {
            char reply[] = "File not found or uninitialized.\n";
            write(client_fd, reply, strlen(reply));
        } else {
            FILE *file = fopen(songList[fileIndex].filePath, "rb");
            if (file == NULL) {
                perror("fopen");
                char reply[] = "Error opening file on server.\n";
                write(client_fd, reply, strlen(reply));
            } else {
                size_t bytesRead;
                while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                    size_t totalSent = 0;
                    while (totalSent < bytesRead) {
                        ssize_t bytesSent = write(
                            client_fd,
                            buffer + totalSent,
                            bytesRead - totalSent
                        );

                        if (bytesSent <= 0) {
                            perror("write");
                            break;
                        }
                        totalSent += bytesSent;
                    }
                }
                fclose(file);
                printf("File transfer complete.\n");
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}