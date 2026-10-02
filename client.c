/* client.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define BUFFER_SIZE 256
#define PORT 5000


int main(void)
{
int client_fd;
struct sockaddr_in server;
char buffer[BUFFER_SIZE] = {0};
/* Create a TCP socket. */
client_fd = socket(AF_INET, SOCK_STREAM, 0);
if (client_fd < 0) { perror("socket"); return 1; }
server.sin_family = AF_INET;
server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
server.sin_port = htons(PORT);

/* Connect to the server on localhost:5000. */

if (connect(client_fd,(struct sockaddr *)&server,sizeof(server)) < 0)
{
perror("connect");
close(client_fd);
return 1;
}

/* Send a message. */
printf("Enter a song to request: ");
char message[100];
scanf("%s", message);
write(client_fd, message, strlen(message) + 1);

FILE *new = fopen("recieved.flac", "wb");  //god bless flac
  ssize_t bytes_received;
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) { //while there is still more data keep writing to the file.
        fwrite(buffer, 1, bytes_received, new);

    }
fclose(new);


close(client_fd);
return 0;
}