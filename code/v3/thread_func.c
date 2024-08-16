#include "CustomFunction.h"
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
#include "variables.h"


void *thread_function(void *arg)
{
    while (1)
    {
        int *pClient;
        pthread_mutex_lock(&queue_mutex);
        while (taskcount == 0)
        {
            pthread_cond_wait(&taskReady, &queue_mutex);
        }
        if (taskcount > 0)
        {
            found = 1;
            pClient = dequeue();
            taskcount--;
            pthread_cond_signal(&taskReady);
        }
        pthread_mutex_unlock(&queue_mutex);
        if (found == 1)
        {
            start_function(pClient);
        }
    }
}
