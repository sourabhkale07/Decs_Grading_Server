#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <fcntl.h>

void error(char *msg)
{
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  double totalResponseTime = 0;
  char *buffer = NULL;
  long file_size;
  struct timeval startTime, endTime, loopStart, loopEnd;
  ;

  if (argc != 6)
  {
    fprintf(stderr, "usage %s hostname serverIP:port sourceCodeFileTobeGraded loopNum sleepTimeSeconds\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);
  int loopCount = atoi(argv[4]);
  int sleepTIME = atoi(argv[5]);

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

  gettimeofday(&loopStart, NULL);
  char buff[10000];

  n = write(sockfd, &loopCount, sizeof(int));
  if (n < 0)
    error("ERROR writing to socket");

  for (int i = 0; i < loopCount; i++)
  {
    memset(buff, 0, sizeof(buff));
    FILE *file = fopen(argv[3], "rb");
    if (file == NULL)
    {
      error("ERROR opening file");
    }

    // Calculate the file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Allocate memory for the buffer based on file size
    buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL)
    {
      error("ERROR allocating memory");
    }

    // Read the entire file into the buffer
    size_t bytesRead = fread(buffer, 1, file_size, file);
    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    fclose(file);

    // SendTime the content of the file to the server
    gettimeofday(&startTime, NULL);
    n = write(sockfd, buffer, bytesRead);
    if (n < 0)
      error("ERROR writing to socket");

    // Read and display the server response
    n = read(sockfd, buff, 10000);
    if (n < 0)
      error("ERROR reading from socket");
    gettimeofday(&endTime, NULL);
    double responseTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1e6;
    totalResponseTime = totalResponseTime + responseTime;
    buff[n] = '\0'; // Null-terminate the received data
    printf("Server response: %s\n", buff);
    if (i < loopCount - 1)
      sleep(sleepTIME);

    // Free allocated memory
    free(buffer);
  }

  gettimeofday(&loopEnd, NULL);
  double loopTime = (loopEnd.tv_sec - loopStart.tv_sec) + (loopEnd.tv_usec - startTime.tv_usec) / 1e6;
  double tr = totalResponseTime / loopCount;
  double thr = (loopCount) / totalResponseTime;
  printf("SUCCESSFUL RESPONSES: %d , AVG RESPONSE TIME: %lf microseconds , THROUGHPUT: %lf", loopCount, tr, thr);

  return 0;
}
