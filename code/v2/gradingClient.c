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
    int sock_fd, port_no, num;
    struct timeval start_time, end_time;
    struct timeval loop_Start, loop_End;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    double response_Time_Total = 0;
    char *buffer = NULL;
    long file_size;
    int count_timeout = 0;
    int error_count = 0;

    if (argc != 7)
    {
        fprintf(stderr, "usage %s <hostname>  <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <time_out-seconds>\n", argv[0]);
        exit(0);
    }

    port_no = atoi(argv[2]);
    int loop_Num = atoi(argv[4]);
    int sleep_Time = atoi(argv[5]);
    int Successfull_Requests = atoi(argv[4]);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port_no);

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }

    struct timeval time_out;
    time_out.tv_sec = atoi(argv[6]); // Set the time_out to 5 seconds
    time_out.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out)) < 0)
    {
        count_timeout++;
        perror("time_out");
    }

    gettimeofday(&loop_Start, NULL);
    char buffer_new[10000];

    num = write(sock_fd, &loop_Num, sizeof(int));
    if (num < 0)
        error("ERROR writing to socket");

    for (int i = 0; i < loop_Num; i++)
    {
        memset(buffer_new, 0, sizeof(buffer_new));
        FILE *fd = fopen(argv[3], "rb");
        if (fd == NULL)
        {
            error("ERROR opening fd");
        }

        // Calculate the file size
        fseek(fd, 0, SEEK_END);
        file_size = ftell(fd);
        rewind(fd);

        // Allocate memory for the buffer based on file size
        buffer = (char *)malloc(file_size + 1);
        if (buffer == NULL)
        {
            error("ERROR allocating memory");
        }

        // Read the entire file into the buffer
        size_t bytes_Read = fread(buffer, 1, file_size, fd);
        buffer[bytes_Read] = '\0'; // Null-terminate the buffer

        fclose(fd);

        // Send_time the content of the file to the server
        gettimeofday(&start_time, NULL);
        num = write(sock_fd, buffer, bytes_Read);
        if (num < 0)
            perror("ERROR writing to socket");

        // Read and display the server response
        num = read(sock_fd, buffer_new, 10000);
        if (num < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf("time_out occurred. Decrementing successful requests.\n");
                Successfull_Requests--;
            }
            else
            {
                perror("ERROR reading from socket");
            }
        }
        gettimeofday(&end_time, NULL);
        double response_Time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6;
        response_Time_Total = response_Time_Total + response_Time;
        buffer_new[num] = '\0'; // Null-terminate the received data
        printf("Server response: %s\n", buffer_new);
        if (i < loop_Num - 1)
            sleep(sleep_Time);

        // Free allocated memory
        free(buffer);
    }

    gettimeofday(&loop_End, NULL);
    error_count = loop_Num - Successfull_Requests;
    int Rate_of_requests;
    Rate_of_requests = loop_Num / response_Time_Total;
    int error_rate = error_count / response_Time_Total;
    int timed_out_rate = count_timeout / response_Time_Total;
    double time_of_Loop = (loop_End.tv_sec - loop_Start.tv_sec) + (loop_End.tv_usec - start_time.tv_usec) / 1e6;
    double avg_response_Time = response_Time_Total / loop_Num;
    double throughput = (Successfull_Requests) / response_Time_Total;
    printf("SUCCESSFUL RESPONSES: %d , AVG RESPONSE TIME: %lf microseconds , THROUGHPUT: %lf , request rate: %d , Requests: %d , Error_rate: %d , time_out_rate: %d , Total time: %lf", Successfull_Requests, avg_response_Time, throughput, Rate_of_requests, loop_Num, error_rate, timed_out_rate, response_Time_Total);

    return 0;
}
