#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // Required for inet_pton

#define BUFFER_SIZE 8192 // I
#define PORT 5000

int main(void) {
    int client_fd;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE] = {0};

    /* Create a TCP socket. */
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { 
        perror("socket"); 
        return 1; 
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "10.1.36.226", &server.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return 1;
    }

    /* Connect to the server on port 5000. */
    if (connect(client_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(client_fd);
        return 1;
    }

    /* Prompt user for song choice */
    printf("Enter a song to request: ");
    char message[100];
    if (scanf("%99s", message) != 1) {
        fprintf(stderr, "Failed to read input.\n");
        close(client_fd);
        return 1;
    }

    /* Send request string WITHOUT the trailing null byte '\0' */
    if (write(client_fd, message, strlen(message)) < 0) {
        perror("write");
        close(client_fd);
        return 1;
    }

    /* Receive binary audio stream from server */
    FILE *file = fopen("received.flac", "wb");
    if (file == NULL) {
        perror("fopen");
        close(client_fd);
        return 1;
    }

    ssize_t bytes_received;
    size_t total_bytes = 0;

    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes += bytes_received;
    }

    if (bytes_received < 0) {
        perror("recv");
    }

    printf("Finished! Downloaded %zu bytes to received.flac\n", total_bytes);

    fclose(file);
    close(client_fd);
    return 0;
}