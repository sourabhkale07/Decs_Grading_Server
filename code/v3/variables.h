#ifndef SHARED_H
#define SHARED_H

#define QUEUE_SIZE 50
#include <pthread.h>

extern void *thread_function(void *arg);
extern int *enqueue(int *client_sockfd);
extern void masterFunction(int *pClient);
extern int *dequeue();
extern int q_size();
extern int front, rear;
extern int count;
extern int taskcount;
extern int found;
extern int *queue[QUEUE_SIZE];
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t taskReady;

#endif 