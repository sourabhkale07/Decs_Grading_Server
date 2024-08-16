#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// Function to serve each client connection
void serve_client(int client_socket)
{
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_in;
    // getting source code from client
    bytes_in = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_in < 0)
    {
        perror("ERROR source code not received");
        close(client_socket);
        return;
    }
    int source_fd = open("temp.c", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (source_fd < 0)
    {
        perror("ERROR source file not created");
        close(client_socket);
        return;
    }
    if (ftruncate(source_fd, 0) == -1)
    {
        // Handle the error if truncation fails
        perror("ftruncate");
        close(source_fd);
        return;
    }
    // Write received source code to the temporary file
    ssize_t bytes_written = write(source_fd, buffer, bytes_in);
    close(source_fd);
    if (bytes_written < 0)
    {
        perror("ERROR writing to source file");
        close(client_socket);
        return;
    }
    int check = system("gcc temp.c -o temp 2>compileError.txt");
    if (check == 0)
    {
        int flag = system("./temp 1>actualOutput.txt 2>error.txt");
        if (flag == 0)
        {
            char Reply[10000];
            memset(Reply, 0, sizeof(Reply));
            char t[35] = "OUTPUT ERROR\n";
            strcat(Reply, t);
            char t2[40] = "\nThe output of 'diff' command is:\n";
            strcat(Reply, t2);
            int flag = system("diff actualOutput.txt expected_output.txt 1>diffError.txt");
            if (flag != 0)
            {
                FILE *errorDFile = fopen("diffError.txt", "r");
                if (errorDFile == NULL)
                {
                    perror("Failed to open output files");
                    // Handle the error as needed
                }
                else
                {
                    char diff_buffer[1024]; // Adjust the buffer size as needed
                    memset(diff_buffer, 0, sizeof(diff_buffer));
                    size_t errorDiffLength = fread(diff_buffer, 1, sizeof(diff_buffer), errorDFile);
                    fclose(errorDFile);
                    if (errorDiffLength < 0)
                    {
                        perror("Error reading expected_output.txt");
                    }
                    else
                    {
                        diff_buffer[errorDiffLength] = '\0'; // Null-terminate the buffer
                    }
                    strcat(Reply, diff_buffer);
                    ssize_t m = send(client_socket, Reply, sizeof(Reply), 0);
                    return;
                }
            }
            else
            {
                send(client_socket, "PASS", sizeof("PASS"), 0);
            }
        }
        else
        {
            FILE *errorFileD = fopen("error.txt", "r");
            if (errorFileD == NULL)
            {
                perror("Failed to open error files");
                // Handle the error as needed
            }
            else
            {
                char errorOutputBuffer[1024]; // Adjust the buffer size as needed
                memset(errorOutputBuffer, 0, sizeof(errorOutputBuffer));
                size_t errorOutputLength = fread(errorOutputBuffer, 1, sizeof(errorOutputBuffer), errorFileD);
                fclose(errorFileD);

                if (errorOutputLength < 0)
                {
                    perror("Error reading error.txt");
                    // Handle the error as needed
                }
                else
                {
                    errorOutputBuffer[errorOutputLength] = '\0'; // Null-terminate the buffer
                }
                send(client_socket, errorOutputBuffer, sizeof(errorOutputBuffer), 0);
                return;
            }
        }
    }
    else
    {
        FILE *errorCFd = fopen("compileError.txt", "r");
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
            return;
        }
    }

    // close(client_socket);
    return;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);
    int loopCount;

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            continue; // Skip this connection and continue listening
        }
        int n = read(newsockfd, &loopCount, sizeof(int));
        if (n < 0)
        {
            perror("ERROR : number of iteration of loop\n");
        }

        while (loopCount > 0)
        {
            serve_client(newsockfd);
            loopCount--;
        }
        close(newsockfd);
    }
    return 0;
}
