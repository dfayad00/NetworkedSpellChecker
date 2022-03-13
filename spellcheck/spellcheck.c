#include "spellcheck.h"

#define DEFAULT_DICTIONARY "dictionary.txt"
#define DEFAULT_PORT 8088

void get_dictionary_size(char *fileName) {
    FILE *fptr = fopen(fileName, "r");
    if(fptr == NULL) {
        perror("couldn't open file");
        exit(1);
    }
    
    size = 0;
    for(char c = getc(fptr); c != EOF; c = getc(fptr))
        if(c == '\n')
            size++;
    
    fclose(fptr);
}

void get_dictionary(char *fileName) {
    dictionary = malloc(size * sizeof(char*) + 1);
    if(dictionary == NULL) {
        perror("allocation error");
        exit(1);
    }
    
    FILE *fptr = fopen(fileName, "r");
    if(fptr == NULL) {
        perror("couldn't open file");
        exit(1);
    }
    
    char line[256];
    int i = 0;
    while(fgets(line, sizeof line, fptr) != NULL) {
        dictionary[i] = (char*) malloc(strlen(line) * sizeof(char) + 1);
        if(dictionary[i] == NULL) {
            perror("allocation error");
            exit(1);
        }
        
        line[strlen(line) - 1] = '\0';
        strcpy(dictionary[i], line);
        i++;
    }
    
    fclose(fptr);
}

int open_listenfd(int port) {
    int listenfd = 0;
    int optval = 1;
    struct sockaddr_in server_addr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

     if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
         return -1;

     bzero((char *) &server_addr, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     server_addr.sin_port = htons((unsigned short)port);
    
     if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
         return -1;

     if (listen(listenfd, 20) < 0)
         return -1;

     return listenfd;
}

Node *create_node(struct sockaddr_in client, char *word, int socket) {
    Node *temp = (Node *) malloc(sizeof(Node));
    if(temp == NULL) {
        perror("allocation error");
        exit(1);
    }
    
    temp->client = client;
    
    if(word == NULL) {
        temp->word = word;
    } else {
        temp->word = malloc(sizeof(char *) * strlen(word) + 1);
        if(temp->word == NULL) {
            perror("allocation error");
            exit(1);
        }
        strcpy(temp->word, word);
    }
    
    temp->next = NULL;
    temp->sock_client = socket;
    return temp;
}

void push(Queue *queue, struct sockaddr_in client, char *word, int socket) {
    Node *temp = create_node(client, word, socket);
    if(queue->queue_size == 0) {
        queue->front = temp;
    } else {
        Node * head = queue->front;
        while(head->next != NULL)
            head = head->next;
        
        head->next = temp;
    }
    queue->queue_size++;
}

Node *pop(Queue *queue) {
    if (queue->front == NULL) {
        queue->queue_size = 0;
        return NULL;
    }

    Node *temp = queue->front;
    queue->front = queue->front->next;
    queue->queue_size--;
    free(queue->front);
    return temp;
}

Queue *create_queue() {
    Queue *temp = (Queue *) malloc(sizeof(Queue));
    if(temp == NULL) {
        perror("allocation error");
        exit(1);
    }
    temp->front = NULL;
    temp->queue_size = 0;
    return temp;
}

void del_queue(Queue *queue) {
    free(queue->front);
    free(queue);
}


int main(int argc, char **argv) {
    char *dictionaryName;
    int port;
    
    //assigns dictionary name and port accordingly
    if(argc == 1) {
        dictionaryName = DEFAULT_DICTIONARY;
        port = DEFAULT_PORT;
      } else if(argc == 2) {
          dictionaryName = argv[1];
          port = DEFAULT_PORT;
      } else {
          dictionaryName = argv[1];
          port = atoi(argv[2]);
      }
    
    if (port < 1024 || port > 65535) {
      printf("invalid port, must be between 1024 and 65535\n");
      exit(1);
    }
    
    //initialize dictionary
    get_dictionary_size(dictionaryName);
    get_dictionary(dictionaryName);

    //initialize queues, locks, and condition variables
    pthread_mutex_init(&jobqLock, NULL);
    pthread_mutex_init(&logqLock, NULL);
    pthread_mutex_init(&logLock, NULL);
    pthread_cond_init(&c1, NULL);
    pthread_cond_init(&c2, NULL);
    pthread_cond_init(&c3, NULL);
    pthread_cond_init(&c4, NULL);
    
   //create worker and log threads
    pthread_t worker[3];
    for (int i = 0; i < 3; i++) {
      pthread_create(&worker[i], NULL, &thread_worker, NULL);
    }

    pthread_t log;
    pthread_create(&log, NULL, &thread_log, NULL);
    
    struct sockaddr_in client;
    socklen_t client_size = sizeof(struct sockaddr_in);
    int connection_socket = open_listenfd(port);
    
    char *connected = "connected (-1 to exit)\n";
    char *full = "queue full\n";

    while(1) {
        int sock_client = accept(connection_socket, (struct sockaddr*)&client, &client_size);
        if (sock_client == -1) {
            printf("error connecting to socket %d\n", sock_client);
            continue;
        }

        pthread_mutex_lock(&jobqLock);
        
        if(jobq->queue_size >= 1000) {
            send(sock_client, full, strlen(full), 0);
            pthread_cond_wait(&c2, &jobqLock);
        }

        printf("connected (client ID: %d)\n", sock_client);
        send(sock_client, connected, strlen(connected), 0);

        push(jobq, client, NULL, sock_client);
        pthread_mutex_unlock(&jobqLock);
        pthread_cond_signal(&c1);
    }
}
