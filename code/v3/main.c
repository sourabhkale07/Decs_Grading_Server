/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "CustomFunction.h"
#include "variables.h"

#define QUEUE_SIZE 50


void *thread_function(void *arg);
int *enqueue(int *client_sockfd);
void masterFunction(int *pClient);
int *dequeue();
int q_size();
int front = 0, rear = 0;
int count = 0;
int taskcount = 0;
int found = 0;
int *queue[QUEUE_SIZE];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskReady = PTHREAD_COND_INITIALIZER;


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 3)
    {
        fprintf(stderr, "Usage, %s <Port No.> <Number of threads>\n",argv[0]);
        exit(1);
    }
    int MAX_THREADS=atoi(argv[2]);
    pthread_t threadPool[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threadPool[i], NULL, thread_function, NULL);
    }

    /*    pthread_t manager_thread;
        pthread_create(&manager_thread, NULL, (void *(*)(void *))manage_thread_pool, NULL);
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 3000); // Set the backlog to a high number

    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        int *pClient = malloc(sizeof(int));
        *pClient = newsockfd;
        masterFunction(pClient);
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&taskReady);
    close(sockfd);

    return 0;
}
