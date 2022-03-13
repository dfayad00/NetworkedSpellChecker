#include "spellcheck.h"
#include <pthread.h>

void *thread_worker(void *args) {
  char *print_prompt = "Enter a word to check spelling: ";
  char *print_close = "Closed connection\n";
  char *print_error = "Error\n";

    while (1) {
        pthread_mutex_lock(&jobqLock);
        if (jobq->queue_size <= 0)
            pthread_cond_wait(&c1, &jobqLock);

        Node *job = pop(jobq);
        pthread_mutex_unlock(&jobqLock);
        pthread_cond_signal(&c2);

        int sock_client = job->sock_client;

        while (1) {
            char recv_buffer[256] = "";
            send(sock_client, print_prompt, strlen(print_prompt), 0);
            int bytes_returned = recv(sock_client, recv_buffer, 256, 0);

            if (bytes_returned <= -1) {
            send(sock_client, print_error, strlen(print_error), 0);
            continue;

            } else if (atoi(&recv_buffer[0]) == -1) {
            send(sock_client, print_close, strlen(print_close), 0);
            close(sock_client);
            break;
                
            } else {
                recv_buffer[strlen(recv_buffer) - 1] = '\0';
                recv_buffer[bytes_returned - 2] = '\0';

                char *result = " OK\n";
                for (int i = 0; i < size; i++) {
                    if (strcmp(recv_buffer, dictionary[i]) == 0) {
                    result = " MISPELLED\n";
                    break;
                    }
                }

                strcat(recv_buffer, result);
                printf("%s", recv_buffer);

                send(sock_client, recv_buffer, strlen(recv_buffer), 0);

                struct sockaddr_in client = job->client;

                pthread_mutex_lock(&logqLock);

                if(logq->queue_size >= 1000) {
                  pthread_cond_wait(&c4, &logqLock);
                }

                push(logq, client, recv_buffer, sock_client);
                pthread_mutex_unlock(&logqLock);
                pthread_cond_signal(&c3);
            }
        }
    }
}

void *thread_log(void *args) {
    while(1) {
        pthread_mutex_lock(&logqLock);
            
        if (logq->queue_size <= 0)
          pthread_cond_wait(&c3, &logqLock);

        Node *node = pop(logq);
        char *word = node->word;

        pthread_mutex_unlock(&logqLock);
        pthread_cond_signal(&c4);

        if (word == NULL)
          continue;

        pthread_mutex_lock(&logLock);

        //write log file
        FILE *log_file = fopen("log.txt", "a");
        fprintf(log_file, "%s", word);
        fclose(log_file);

        pthread_mutex_unlock(&logLock);
    }
}
