#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define PORT 5000
#define BUFFER_SIZE 256


typedef struct{
    char songName[50]; //this is a char array (string) for titles
    char filePath[100]; //this is a string for the file path
} Song;

//this function checks if the file name matches then returns the file pointer for the server to read and send from.
int findFile(char *filename, Song songList[], int size) { //time complexity of O(n) ): i dont feel like making a hash table. 
    for (int i = 0; i < size; i++) {
        if (strcmp(songList[i].songName, filename) == 0) {
            return i; // Return the index of the found song
        }
    }
    return -1; // Return -1 if the file is not found
}


int main(void)
{

int server_fd, client_fd;
struct sockaddr_in server_addr;
char buffer[BUFFER_SIZE];
Song songList[10] = {0};// this is the array of 10 songs to pick from

// this is just some test data 
strcpy(songList[0].songName, "The");
strcpy(songList[0].filePath, "The.flac");

/* Create a TCP socket. */
server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd == -1)
{
perror("socket");
return 1;
}

/* Configure the server address. */
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(PORT);

/* Bind the socket to the port. */
if (bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
{
perror("bind");
close(server_fd);
return 1;
}

while (1) {
/* Listen for incoming client connections. */
if (listen(server_fd, 5) == -1)
{
perror("listen");
close(server_fd);
return 1;
}
printf("File Server - PID: %d\n", getpid());
printf("Waiting for a client on port %d...\n", PORT);

/* Wait until a client connects. */
client_fd = accept(server_fd, NULL, NULL);
if (client_fd == -1)
{
perror("accept");
close(server_fd);
return 1;
}
printf("Client connected.\n");

/* Receive a message from the client. */
int n = read(client_fd, buffer, BUFFER_SIZE - 1); //n is the number of bytes read, not the data itself
if (n > 0)
{
buffer[n] = '\0';
printf("Received: %s\n", buffer);
}
    int fileIndex = findFile(buffer, songList, 10); //this is the function to find the file

    FILE *file = fopen(songList[fileIndex].filePath, "rb");
if (fileIndex == -1) {
        char reply[] = "File not found or uninitialized.";
        write(client_fd, reply, strlen(reply) + 1);
    } else {
        char reply[] = "File found! Sending...";
        size_t bytesRead;
       while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            write(client_fd, buffer, bytesRead);
        }

        write(client_fd, reply, strlen(reply) + 1);
        // code to read from 'file' and write to client_fd would go here
    }
printf("Response sent.\n");
fclose(file);
close(client_fd);
//note that this is blocking and single threaded because there is no thread pool needed to manage multiple clients.

}
return 0;
}