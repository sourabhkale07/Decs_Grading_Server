#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>

using namespace std;

struct sockaddr_in serverAddr;
int clientSocket;
const int MAX_BUFFER_SIZE = 10000;
pthread_mutex_t lc;
int req=0,timeout=0,errors=0,suc=0;
int polling_interval=1;

int main(int argc, char* argv[]) 
{
    pthread_mutex_init(&lc,NULL);
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <request | status> <serverIP:port> <sourceCodeFileTobeGraded | request_id>  " << endl;
        return 1;
    }
    string type=argv[1];
    char* serverAddress = strtok(argv[2], ":");
    char* serverPort = strtok(NULL, ":");
    
    char buffer[MAX_BUFFER_SIZE];
    timeval Tsend,Trecv,diff,sum,ti,te;
    sum.tv_sec=0,sum.tv_usec=0;
    memset(buffer, 0, sizeof(buffer)); 
    
    if(type=="request")
    {
        char* sourceCodeFile = argv[3];
        ifstream file(sourceCodeFile);
        string sourceCode((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();
        
        if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            cerr << "Error creating socket." << endl;
            return 1;
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(serverPort));
        serverAddr.sin_addr.s_addr = inet_addr(serverAddress);
        if ( connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
        {            
            return 0;
        }
        
        // Send the source code to the server
        ssize_t bytesSent = send(clientSocket, "request", sizeof("request"), 0);
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0); 
        bytesSent = send(clientSocket, sourceCode.c_str(), sourceCode.size(), 0);
        if (bytesSent == -1 || bytesSent == 0) {
            return 0;
        }
        else req++;  
        memset(buffer, 0, sizeof(buffer));

        long long int request_id;
        
        // Receive requestID from the server
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) 
        {
            return 0;
        }
        // recv(clientSocket, &request_id, sizeof(request_id),0) ;
        
        cout << "Here is your request ID: "<<buffer << endl;
        sleep(1);
    }
    else
    {
        char* req_id = argv[3];
        if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            cerr << "Error creating socket." << endl;
            return 1;
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(serverPort));
        serverAddr.sin_addr.s_addr = inet_addr(serverAddress);
        if ( connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
        {
            
            return 0;
        }

        // Send the source code to the server
        while(true)
        {
            ssize_t bytesSent = send(clientSocket, "status", sizeof("status"), 0);
            if (bytesSent == -1 || bytesSent == 0) 
            {
                continue;	 
            }
            else break;
        }
        while(true)
        {
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            // cout<<"check 1: "<<buffer<<endl;
            if (bytesRead == -1 || bytesRead == 0) {
                continue;	           
            }
            else break;
        }

        // gettimeofday(&Tsend, NULL);
        
        while(true)
        {
            ssize_t bytesSent = send(clientSocket, req_id, strlen(req_id), 0);
            if (bytesSent == -1 || bytesSent == 0) {
                    continue;
            }
            else req++;
            
            memset(buffer, 0, sizeof(buffer));
            
            // Receive result from the server
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) 
            {
                
                close(clientSocket);
                return 0;
            }
            // else if (strcmp(buffer, "DONE") == 0) 
            // { 
            //     // suc++;
            //     bytesSent = send(clientSocket, "please send the result", sizeof("please send the result"), 0);
            //     memset(buffer, 0, sizeof(buffer));
            //     bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0); 
            //     cout<<"check 3: "<<buffer<<endl;        
            //     // gettimeofday(&Trecv, NULL);       
            //     break;
            // }
            // else if (strcmp(buffer, "Invalid request ID given") == 0) 
            // {	
            //     cout<<"Invalid request ID given\n";
            //     break;
            // }
            else 
            {
                cout<<buffer<<endl;
                break;
            }
        }
        }
    close(clientSocket);
    return 0;
}