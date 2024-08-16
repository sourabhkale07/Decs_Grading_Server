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
#include <pthread.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

pthread_mutex_t mutex_lock;

void *start_Function(void *sock_fd)
{

    int socket_of_Client = *(int *)sock_fd;
    int loop_Num;
    read(socket_of_Client, &loop_Num, sizeof(int));
    pthread_mutex_unlock(&mutex_lock);

    for (int i = 0; i < loop_Num; i++)
    {
        char buffer[4096];
        char temporary_fileName[30];
        memset(temporary_fileName, 0, sizeof(temporary_fileName));
        snprintf(temporary_fileName, 30, "temp_%d.c", gettid());

        char opt_fileName[30];
        memset(opt_fileName, 0, sizeof(opt_fileName));
        snprintf(opt_fileName, 30, "temp_%d", gettid());

        memset(buffer, 0, sizeof(buffer));
        ssize_t in_Bytes;
        // getting source code from client

        in_Bytes = read(socket_of_Client, buffer, sizeof(buffer));

        if (in_Bytes <= 0)
        {

            perror("ERROR source code not received");
            close(socket_of_Client);
        }

        int fd_of_SourceFile = open(temporary_fileName, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (fd_of_SourceFile < 0)
        {

            perror("ERROR source file not created");
            close(socket_of_Client);
        }

        if (ftruncate(fd_of_SourceFile, 0) == -1)
        {

            // Handle the error if truncation fails
            perror("ftruncate");
            close(fd_of_SourceFile);
            close(socket_of_Client);
        }

        // Write received source code to the temporary file
        ssize_t written_Bytes = write(fd_of_SourceFile, buffer, in_Bytes);
        close(fd_of_SourceFile);

        if (written_Bytes < 0)
        {

            perror("ERROR writing to source file");
            close(socket_of_Client);
        }
        char err_fileName[30];
        memset(err_fileName, 0, sizeof(err_fileName));
        snprintf(err_fileName, 30, "errorfile_%d.txt", gettid()); // puts string into buffer

        char actual_fileName[30];
        memset(actual_fileName, 0, sizeof(actual_fileName));
        snprintf(actual_fileName, 30, "actualOutput_%d.txt", gettid());

        char runtime_errorFile[30];
        memset(runtime_errorFile, 0, sizeof(runtime_errorFile));
        snprintf(runtime_errorFile, 30, "runtimeError_%d.txt", gettid());

        char cmd[100];

        snprintf(cmd, 100, "gcc %s -o %s 2>%s", temporary_fileName, opt_fileName, err_fileName);
        int cmd_check = system(cmd);
        if (cmd_check == 0)
        {

            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, 100, "./%s 1>%s 2>%s", opt_fileName, actual_fileName, runtime_errorFile);
            // int cmd_flag = system("./temp 1>actualOutput.txt 2>runtime_Error_Buffer");
            int cmd_flag = system(cmd);
            if (cmd_flag == 0)
            {

                char msg_Reply[10000];
                memset(msg_Reply, 0, sizeof(msg_Reply));
                char t[35] = "OUTPUT ERROR\n";
                strcat(msg_Reply, t);
                char t2[40] = "\nThe output of 'diff' cmd is:\n";
                strcat(msg_Reply, t2);

                char diff_Error_fileName[30];
                memset(diff_Error_fileName, 0, sizeof(diff_Error_fileName));
                snprintf(diff_Error_fileName, 30, "diffErrorfile_%d.txt", gettid());
                snprintf(cmd, 100, "diff %s expected_output.txt 1>%s", actual_fileName, diff_Error_fileName);
                // int cmd_flag = system("diff actualOutput.txt expected_output.txt 1>errorD.txt");
                cmd_flag = system(cmd);
                if (cmd_flag != 0)
                {

                    FILE *errd_File = fopen(diff_Error_fileName, "r");
                    if (errd_File == NULL)
                    {
                        perror("Failed to open output files");
                        // Handle the error as needed
                    }
                    else
                    {

                        char errd_Buffer[1024]; // Adjust the buffer size as needed
                        memset(errd_Buffer, 0, sizeof(errd_Buffer));
                        size_t errd_Length = fread(errd_Buffer, 1, sizeof(errd_Buffer), errd_File);
                        fclose(errd_File);
                        if (errd_Length < 0)
                        {
                            perror("Error reading expected_output.txt");
                        }
                        else
                        {
                            errd_Buffer[errd_Length] = '\0'; // Null-terminate the buffer
                        }
                        strcat(msg_Reply, errd_Buffer);
                        ssize_t ma = send(socket_of_Client, msg_Reply, sizeof(msg_Reply), 0);
                        remove(diff_Error_fileName);
                        remove(err_fileName);
                        remove(runtime_errorFile);
                        remove(actual_fileName);
                        remove(temporary_fileName);
                        remove(opt_fileName);
                    }
                }
                else
                {

                    send(socket_of_Client, "PASS", sizeof("PASS"), 0);
                    remove(diff_Error_fileName);
                    remove(err_fileName);
                    remove(runtime_errorFile);
                    remove(actual_fileName);
                    remove(temporary_fileName);
                    remove(opt_fileName);
                }
            }
            else
            {

                FILE *err_Fd = fopen(runtime_errorFile, "r");
                if (err_Fd == NULL)
                {
                    perror("Failed to open error files");
                    // Handle the error as needed
                }
                else
                {

                    char err_Obuffer[1024]; // Adjust the buffer size as needed
                    memset(err_Obuffer, 0, sizeof(err_Obuffer));
                    size_t err_opt_Lenght = fread(err_Obuffer, 1, sizeof(err_Obuffer), err_Fd);
                    fclose(err_Fd);

                    if (err_opt_Lenght < 0)
                    {
                        perror("Error reading error.txt");
                        // Handle the error as needed
                    }
                    else
                    {
                        err_Obuffer[err_opt_Lenght] = '\0'; // Null-terminate the buffer
                    }

                    send(socket_of_Client, err_Obuffer, sizeof(err_Obuffer), 0);
                    remove(err_fileName);
                    remove(runtime_errorFile);
                    remove(actual_fileName);
                    remove(temporary_fileName);
                    remove(opt_fileName);
                }
            }
        }
        else
        {

            FILE *err_CFd = fopen(err_fileName, "r");
            if (err_CFd == NULL)
            {
                perror("Failed to open error files");
                // Handle the error as needed
            }
            else
            {

                char errc_OBuffer[1024]; // Adjust the buffer size as needed
                size_t errc_opt_Lengh = fread(errc_OBuffer, 1, sizeof(errc_OBuffer), err_CFd);
                fclose(err_CFd);

                if (errc_opt_Lengh < 0)
                {
                    perror("Error reading error.txt");
                    // Handle the error as needed
                }
                else
                {
                    errc_OBuffer[errc_opt_Lengh] = '\0'; // Null-terminate the buffer
                }
                send(socket_of_Client, errc_OBuffer, sizeof(errc_OBuffer), 0);

                remove(err_fileName);
                remove(runtime_errorFile);
                remove(actual_fileName);
                remove(temporary_fileName);
                remove(opt_fileName);
            }

            // close(socket_of_Client);

            remove(err_fileName);
            remove(runtime_errorFile);
            remove(actual_fileName);
            remove(temporary_fileName);
            remove(opt_fileName);
        }
        remove(err_fileName);
        remove(runtime_errorFile);
        remove(actual_fileName);
        remove(temporary_fileName);
        remove(opt_fileName);
    }
    close(socket_of_Client);
    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    int sock_fd, new_sock_fd, port_no;
    socklen_t cli_length;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    /* create socket */

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
        error("ERROR opening socket");

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
     */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    port_no = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    /* bind socket to this port number on this machine */

    if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    /* listen for incoming connection requests */

    listen(sock_fd, 100);
    cli_length = sizeof(cli_addr);
    pthread_mutex_init(&mutex_lock, NULL);
    
    while (1)
    {
        pthread_mutex_lock(&mutex_lock);
        new_sock_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_length);
        if (new_sock_fd < 0)
            error("ERROR on accept");
        pthread_t thrd;
        if (pthread_create(&thrd, NULL, &start_Function, &new_sock_fd) != 0)
            printf("Failed to create Thread\n");
    }

    return 0;
}
