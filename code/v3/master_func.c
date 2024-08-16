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



void masterFunction(int *pClient)
{
    pthread_mutex_lock(&queue_mutex);
    while ((rear + 1) % QUEUE_SIZE == front)
    {
        pthread_cond_wait(&taskReady, &queue_mutex);
    }
    enqueue(pClient);
    taskcount++;
    pthread_cond_signal(&taskReady);
    pthread_mutex_unlock(&queue_mutex);
}

