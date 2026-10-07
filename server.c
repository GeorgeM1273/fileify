#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#define QUEUE_SIZE 10 
#define NUM_WORKERS 4
#define PORT 5000
#define BUFFER_SIZE 4096

typedef struct {
    char songName[50];
    char filePath[100];
} Song;

typedef struct {
    int client_fd;
} task_t;

typedef struct {
    task_t queue[QUEUE_SIZE];
    int front;
    int rear;
    int count;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_t workers[NUM_WORKERS];
} thread_pool_t;

Song songList[10] = {0};

int findFile(char *filename, Song songList[], int size) {
    for (int i = 0; i < size; i++) {
        if (strcmp(songList[i].songName, filename) == 0) {
            return i;
        }
    }
    return -1;
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE]; // Local thread-safe buffer

    int n = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }

    buffer[n] = '\0';
    buffer[strcspn(buffer, "\r\n")] = 0; // Strip newlines
    printf("[Worker %lu] Requested: %s\n", (unsigned long)pthread_self(), buffer);

    int index = findFile(buffer, songList, 10);
    if (index == -1) {
        const char *not_found_msg = "File not found\n";
        write(client_fd, not_found_msg, strlen(not_found_msg));
        close(client_fd);
        return;
    }

    FILE *file = fopen(songList[index].filePath, "rb");
    if (!file) {
        perror("fopen");
        const char *err_msg = "Error opening file\n";
        write(client_fd, err_msg, strlen(err_msg));
        close(client_fd);
        return; //if there is an error we should return here to avoid the rest of the function
    }

    while ((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        write(client_fd, buffer, n);
    }

    fclose(file);
    close(client_fd);
    printf("[Worker %lu] Transfer complete.\n", (unsigned long)pthread_self());
}

void *worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        task_t task;

        pthread_mutex_lock(&pool->mutex); //lock mutex and wait for job
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->mutex);
        }

        if (pool->count == 0 && pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        } //unloack and break out, returning null

        task = pool->queue[pool->front];
        pool->front = (pool->front + 1) % QUEUE_SIZE; //queue is circular, so we wrap around
        pool->count--; //decrament count of jobs in queue

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->mutex);

        handle_client(task.client_fd); //execute the job.
    }
    return NULL;
}

//initalizes the thread pool and creates the worker threads
void pool_init(thread_pool_t *pool) {
    pool->front = 0;
    pool->rear = 0;
    pool->count = 0;
    pool->shutdown = 0;
    //initalize pool values

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    //create workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&pool->workers[i], NULL, worker, pool);
    }
}

//thread safe enqueue
//we use mutex to act as a syncronizer 
void pool_push(thread_pool_t *pool, int client_fd) {
    pthread_mutex_lock(&pool->mutex);

    while (pool->count == QUEUE_SIZE && !pool->shutdown) {
        pthread_cond_wait(&pool->not_full, &pool->mutex);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return;
    }

    pool->queue[pool->rear].client_fd = client_fd;
    pool->rear = (pool->rear + 1) % QUEUE_SIZE;
    pool->count++;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->mutex);
}

int main(void) {
    //create and initalize the thread pool
    thread_pool_t pool;
    pool_init(&pool);

    int server_fd, client_fd;
    struct sockaddr_in server_addr;

    strcpy(songList[0].songName, "The");
    strcpy(songList[0].filePath, "The.flac");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET; // Use IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(PORT); //assign port to listen on

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

    printf("Fileify running on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        //delagate client management to worker queue
        pool_push(&pool, client_fd);
    }

    close(server_fd);
    return 0;
}