#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct timeval start, end;
    struct timeval loopS, loopE;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    double response_Time_Total = 0;
    char *buffer = NULL;
    long file_size;
    int count_timeout = 0;
    int timeoutCount = 0;
    int errorCount = 0;

    if (argc != 7)
    {
        fprintf(stderr, "usage %s <hostname>  <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);
    int loop_Num = atoi(argv[4]);
    int sTime = atoi(argv[5]);
    int Successfull_Requests = atoi(argv[4]);

    struct timeval timeout;
    timeout.tv_sec = atoi(argv[6]); // Set the timeout to 5 seconds
    timeout.tv_usec = 0;
    gettimeofday(&loopS, NULL);
    for (int i = 0; i < loop_Num; i++)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");

        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }

        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR connecting");

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            count_timeout++;
            perror("time_out");
        }

        char buff[10000];

        /* n = write(sockfd, &loop_Num, sizeof(int));
         if (n < 0)
             error("ERROR writing to socket");*/

        memset(buff, 0, sizeof(buff));
        FILE *file = fopen(argv[3], "rb");
        if (file == NULL)
        {
            errorCount++;
            Successfull_Requests--;
            perror("ERROR opening file");
            continue;
        }

        // Calculate the file size
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        rewind(file);

        // Allocate memory for the buffer based on file size
        buffer = (char *)malloc(file_size + 1);
        if (buffer == NULL)
        {
            errorCount++;
            Successfull_Requests--;
            perror("ERROR allocating memory");
            continue;
        }

        // Read the entire file into the buffer
        size_t bytesRead = fread(buffer, 1, file_size, file);
        buffer[bytesRead] = '\0'; // Null-terminate the buffer

        fclose(file);

        // Send the content of the file to the server
        gettimeofday(&start, NULL);
        n = write(sockfd, buffer, bytesRead);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            errorCount++;
            Successfull_Requests--;
        }
        // Read and display the server response
        n = read(sockfd, buff, 10000);
        if (n < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                errorCount++;
                timeoutCount++;
                Successfull_Requests--;
            }
            else
            {
                errorCount++;
                Successfull_Requests--;
                perror("ERROR reading from socket");
            }
        }
        gettimeofday(&end, NULL);
        double responseTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        response_Time_Total = response_Time_Total + responseTime;
        buff[n] = '\0'; // Null-terminate the received data
        printf("Server response: %s\n", buff);
        if (i < loop_Num - 1)
            sleep(sTime);

        close(sockfd);
        // Free allocated memory
        free(buffer);
    }

    gettimeofday(&loopE, NULL);
    int error_count = loop_Num - Successfull_Requests;
    int Rate_of_requests;
    Rate_of_requests = (loop_Num) / response_Time_Total;
    int error_rate = error_count / response_Time_Total;
    int timed_out_rate = count_timeout / response_Time_Total;
    double time_of_Loop = (loopE.tv_sec - loopS.tv_sec) + (loopE.tv_usec - start.tv_usec) / 1e6;
    double avg_response_Time = response_Time_Total / loop_Num;
    double throughput = (Successfull_Requests) / response_Time_Total;
    printf("SUCCESSFUL RESPONSES: %d , AVG RESPONSE TIME: %lf microseconds , THROUGHPUT: %lf , request rate: %d , Requests: %d , Error_rate: %d , time_out_rate: %d , Total time: %lf", Successfull_Requests, avg_response_Time, throughput, Rate_of_requests, loop_Num, error_rate, timed_out_rate, response_Time_Total);

    return 0;
}
