#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

char **dictionary;
int size;

pthread_mutex_t jobqLock;
pthread_mutex_t logqLock;
pthread_mutex_t logLock;
pthread_cond_t c1;
pthread_cond_t c2;
pthread_cond_t c3;
pthread_cond_t c4;

void *thread_worker(void*);
void *thread_log(void*);

typedef struct Node {
    struct sockaddr_in client;
    int sock_client;
    char *word;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    int queue_size;
} Queue;

Queue *jobq;
Queue *logq;

void delete_queue(Queue*);
Node *create_node(struct sockaddr_in, char*, int);
void push(Queue*, struct sockaddr_in , char*, int);
Node *pop(Queue*);

char **open_dictionary(char*);
int open_listenfd(int);
void *worker_thread(void*);
void *log_thread(void*);
