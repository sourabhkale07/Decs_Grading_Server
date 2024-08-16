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
// pthread_mutex_t assignmentMutex = PTHREAD_MUTEX_INITIALIZER;
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void *start_function(int *sockfd)
{
    int client_socket = *(int *)sockfd;
    // pthread_mutex_unlock(&assignmentMutex);
    char buffer[4096];
    char temp_file_name[30];
    snprintf(temp_file_name, 30, "temp_%d.c", gettid());

    char output_file_name[30];
    snprintf(output_file_name, 30, "temp_%d", gettid());

    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_in;
    // getting source code from client
    bytes_in = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_in < 0)
    {
        perror("ERROR source code not received");
        close(client_socket);
        return NULL;
    }
    int source_fd = open(temp_file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (source_fd < 0)
    {
        perror("ERROR source file not created");
        close(client_socket);
        return NULL;
    }
    if (ftruncate(source_fd, 0) == -1)
    {
        // Handle the error if truncation fails
        perror("ftruncate");
        close(source_fd);
        return NULL;
    }
    // Write received source code to the temporary file
    ssize_t bytes_written = write(source_fd, buffer, bytes_in);
    close(source_fd);
    if (bytes_written < 0)
    {
        perror("ERROR writing to source file");
        close(client_socket);
        return NULL;
    }
    /*char compilerErrorBuffer[1024];
    char runtimeErrorBuffer[1024];
    memset(compilerErrorBuffer, 0, sizeof(compilerErrorBuffer));
    memset(runtimeErrorBuffer, 0, sizeof(runtimeErrorBuffer));*/

    // int check = system("gcc temp.c -o temp 2>compilerErrorBuffer");
    char error_file_name[30];
    memset(error_file_name, 0, sizeof(error_file_name));
    snprintf(error_file_name, 30, "errorfile_%d.txt", gettid()); // puts string into buffer

    char actual_output_file[30];
    memset(actual_output_file, 0, sizeof(actual_output_file));
    snprintf(actual_output_file, 30, "actualOutput_%d.txt", gettid());

    char runtime_error_file[30];
    memset(runtime_error_file, 0, sizeof(runtime_error_file));
    snprintf(runtime_error_file, 30, "runtimeError_%d.txt", gettid());

    char command[100];

    snprintf(command, 100, "gcc %s -o %s 2>%s", temp_file_name, output_file_name, error_file_name);

    int check = system(command);
    if (check == 0)
    {
        memset(command, 0, sizeof(command));
        snprintf(command, 100, "./%s 1>%s 2>%s", output_file_name, actual_output_file, runtime_error_file);
        int flag = system(command);
        if (flag == 0)
        {
            char Reply[10000];
            memset(Reply, 0, sizeof(Reply));
            char t[35] = "OUTPUT ERROR\n";
            strcat(Reply, t);
            char t2[40] = "\nThe output of 'diff' command is:\n";
            strcat(Reply, t2);

            char diffError_file_name[30];
            memset(diffError_file_name, 0, sizeof(diffError_file_name));
            snprintf(diffError_file_name, 30, "diffErrorfile_%d.txt", gettid());
            snprintf(command, 100, "diff %s expected_output.txt 1>%s", actual_output_file, diffError_file_name);
            // int flag = system("diff actualOutput.txt expected_output.txt 1>errorD.txt");
            flag = system(command);
            if (flag != 0)
            {
                FILE *errorDFile = fopen(diffError_file_name, "r");
                if (errorDFile == NULL)
                {
                    perror("Failed to open output files");
                    // Handle the error as needed
                }
                else
                {
                    char error_D_buffer[1024]; // Adjust the buffer size as needed
                    memset(error_D_buffer, 0, sizeof(error_D_buffer));
                    size_t errorDLength = fread(error_D_buffer, 1, sizeof(error_D_buffer), errorDFile);
                    fclose(errorDFile);
                    if (errorDLength < 0)
                    {
                        perror("Error reading expected_output.txt");
                    }
                    else
                    {
                        error_D_buffer[errorDLength] = '\0'; // Null-terminate the buffer
                    }
                    strcat(Reply, error_D_buffer);
                    ssize_t m = send(client_socket, Reply, sizeof(Reply), 0);
                    remove(diffError_file_name);
                    remove(error_file_name);
                    remove(runtime_error_file);
                    remove(actual_output_file);
                    remove(temp_file_name);
                    remove(output_file_name);
                    close(client_socket);
                    return NULL;
                }
            }
            else
            {
                send(client_socket, "PASS", sizeof("PASS"), 0);
                remove(diffError_file_name);
                remove(error_file_name);
                remove(runtime_error_file);
                remove(actual_output_file);
                remove(temp_file_name);
                remove(output_file_name);
                close(client_socket);
                return NULL;
            }
        }
        else
        {
            FILE *errorFd = fopen(runtime_error_file, "r");
            if (errorFd == NULL)
            {
                perror("Failed to open error files");
                // Handle the error as needed
            }
            else
            {
                char error_O_buffer[1024]; // Adjust the buffer size as needed
                memset(error_O_buffer, 0, sizeof(error_O_buffer));
                size_t errorOutputLength = fread(error_O_buffer, 1, sizeof(error_O_buffer), errorFd);
                fclose(errorFd);

                if (errorOutputLength < 0)
                {
                    perror("Error reading error.txt");
                    // Handle the error as needed
                }
                else
                {
                    error_O_buffer[errorOutputLength] = '\0'; // Null-terminate the buffer
                }
                send(client_socket, error_O_buffer, sizeof(error_O_buffer), 0);
                remove(error_file_name);
                remove(runtime_error_file);
                remove(actual_output_file);
                remove(temp_file_name);
                remove(output_file_name);
                close(client_socket);
                return NULL;
            }
        }
    }
    else
    {
        FILE *errorCFd = fopen(error_file_name, "r");
        if (errorCFd == NULL)
        {
            perror("Failed to open error files");
            // Handle the error as needed
        }
        else
        {
            char errorC_O_buffer[1024]; // Adjust the buffer size as needed
            size_t errorCOutputLength = fread(errorC_O_buffer, 1, sizeof(errorC_O_buffer), errorCFd);
            fclose(errorCFd);

            if (errorCOutputLength < 0)
            {
                perror("Error reading error.txt");
                // Handle the error as needed
            }
            else
            {
                errorC_O_buffer[errorCOutputLength] = '\0'; // Null-terminate the buffer
            }
            send(client_socket, errorC_O_buffer, sizeof(errorC_O_buffer), 0);
            remove(error_file_name);
            remove(runtime_error_file);
            remove(actual_output_file);
            remove(temp_file_name);
            remove(output_file_name);
            close(client_socket);
            return NULL;
        }

        remove(error_file_name);
        remove(runtime_error_file);
        remove(actual_output_file);
        remove(temp_file_name);
        remove(output_file_name);
        close(client_socket);
        return NULL;
        // pthread_exit(NULL);
    }

    remove(error_file_name);
    remove(runtime_error_file);
    remove(actual_output_file);
    remove(temp_file_name);
    remove(output_file_name);
    close(client_socket);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 3)
    {
        fprintf(stderr, "Usage, %s <Port No.> <Number of threads>\n", argv[0]);
        exit(1);
    }
    int MAX_THREADS = atoi(argv[2]);
    pthread_t threadPool[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threadPool[i], NULL, thread_function, NULL);
    }
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

int q_size()
{
    if (front == -1 && rear == -1)
    {
        // The queue is empty
        return 0;
    }
    else if (rear >= front)
    {
        // No wrap-around has occurred
        return rear - front + 1;
    }
    else
    {
        // Wrap-around has occurred
        return (QUEUE_SIZE - front + rear + 1) % QUEUE_SIZE;
    }
}
