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


int *enqueue(int *client_sockfd)
{
    rear = (rear + 1) % QUEUE_SIZE;
    if (front == rear)
    {
        printf("Queue is Full \n\n\n\n");
        if (rear = 0)
            rear = QUEUE_SIZE - 1;
        else
            rear--;
        return NULL;
    }
    else
    {
        queue[rear] = client_sockfd;
    }
}


int *dequeue()
{
    int *item;
    if (front == rear)
    {
        printf("Queue is empty\n\n\n");
        return NULL;
    }
    else
    {
        front = (front + 1) % QUEUE_SIZE;
        item = queue[front];
        return item;
    }
}

int q_size() {
    if (front == -1 && rear == -1) {
        // The queue is empty
        return 0;
    } else if (rear >= front) {
        // No wrap-around has occurred
        return rear - front + 1;
    } else {
        // Wrap-around has occurred
        return (QUEUE_SIZE - front + rear + 1) % QUEUE_SIZE;
    }
}
