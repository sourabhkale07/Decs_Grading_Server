#include <iostream>
#include<bits/stdc++.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <queue>

using namespace std;

const string SUBMISSIONS_DIR = "./submissions/";
const string EXECUTABLES_DIR = "./executables/";
const string OUTPUTS_DIR = "./outputs/";
const string FINAL_RESULT_DIR = "./final_result/";
const string RUNTIME_ERROR_DIR = "./runtime_error/";
const string EXPECTED_OUTPUT = "./expected/output.txt";
const string IN_QUEUE= "IN QUEUE";
const string PROCESSING= "PROCESSING";

const string PASS_MSG = "PASS";
const string COMPILER_ERROR_MSG = "COMPILER ERROR";
const string RUNTIME_ERROR_MSG = "RUNTIME ERROR";
const string OUTPUT_ERROR_MSG = "OUTPUT ERROR";
const int BUFFER_SIZE = 10000;
long long int counter=0;

pthread_mutex_t counter_lock, enqueue_lock, dequeue_lock;
pthread_cond_t condition;

map<long long int, string> table;
map<int, long long int> mp;
queue<int> requests;


void makeDirectories(){
    system(("mkdir "+SUBMISSIONS_DIR+" "+EXECUTABLES_DIR+" "+OUTPUTS_DIR+" "+FINAL_RESULT_DIR+" expected").c_str());
}

void requestHandler( long long int request_id, int clientSocket){
    
    char *buffer = new char[BUFFER_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == 0 || bytesRead == -1) {
        
        close(clientSocket);
        return;
    }
    buffer[bytesRead]='\0';
    table[request_id]= PROCESSING;
    string reqIdStr= to_string(request_id);
    // send(clientSocket, ("We have received your request. Here is your request id: "+reqIdStr).c_str(), sizeof("We have received your request. Here is your request id: "+reqIdStr),0);
    send(clientSocket, (reqIdStr).c_str(), sizeof(reqIdStr), 0);
    close(clientSocket);

    string submittedFile= SUBMISSIONS_DIR+to_string(request_id)+".cpp";
    string outputFile= OUTPUTS_DIR+to_string(request_id)+".txt";
    string executableFile= EXECUTABLES_DIR+to_string(request_id);
    string finalFile= FINAL_RESULT_DIR+to_string(request_id)+".txt";

    ofstream openFile(submittedFile);
    openFile<<buffer;
    openFile.close();

    memset(buffer, 0, sizeof(buffer));


    int compileFlag= system(("g++ "+submittedFile+" -o "+executableFile+" 2>"+outputFile).c_str());
    if (compileFlag!=0){
        table[request_id]=COMPILER_ERROR_MSG;
        system(("rm "+submittedFile).c_str());
        return;
    }

    int executionFlag= system(("./"+executableFile+" 1> "+outputFile+" 2> "+finalFile).c_str());
    if(executionFlag!=0){
        table[request_id]= RUNTIME_ERROR_MSG;
        system(("rm "+submittedFile+" "+executableFile).c_str());
        return;
    }

    system(("rm "+submittedFile).c_str());
    ifstream outputFileFD(outputFile);
    string outputString((istreambuf_iterator<char>(outputFileFD)), istreambuf_iterator<char>());
    // string expectedOutputString="1 2 3 4 5 6 7 8 9 10 ";
    ifstream file(EXPECTED_OUTPUT);
    string expectedOutputString((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    // string outputString;
    // getline(outputFileFD, outputString);
    bool isCorrect= (outputString==expectedOutputString);
    cout<<endl<<outputString<<flush;

    if(isCorrect){
        table[request_id]=PASS_MSG;
        system(("rm "+ executableFile).c_str());
        return;
    }

    table[request_id]=OUTPUT_ERROR_MSG;
    system(("diff "+outputFile+" "+EXPECTED_OUTPUT+">"+finalFile).c_str());
    system(("rm "+ outputFile+" "+executableFile).c_str());
    return;

}

void sendStatus(string msg, string filepath, int clientSocket){
    ifstream file(filepath);
    string output((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    ssize_t bytesSent = send(clientSocket, (msg+": ").c_str(), msg.size()+2, 0);
    bytesSent = send(clientSocket, output.c_str(), output.size(), 0);
    if (bytesSent == -1 || bytesSent == 0) {
        return ;
    }

}

void statusHandler(int clientSocket){
    char *buffer = new char[BUFFER_SIZE];
    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == 0 || bytesRead == -1) {
        
        close(clientSocket);
        return;
    }
    buffer[bytesRead]='\0';
    char* req_pointer;
    long long int request_id= strtoll(buffer, &req_pointer, 10);
    string outputFile= OUTPUTS_DIR+to_string(request_id)+".txt";
    string finalFile= FINAL_RESULT_DIR+to_string(request_id)+".txt";

    if(table.find(request_id)!=table.end()){
        if(table[request_id]==PASS_MSG) sendStatus(PASS_MSG, outputFile, clientSocket);
        else if(table[request_id]==COMPILER_ERROR_MSG) sendStatus(COMPILER_ERROR_MSG, outputFile, clientSocket);
        else if(table[request_id]==RUNTIME_ERROR_MSG) sendStatus(RUNTIME_ERROR_MSG, finalFile, clientSocket);
        else if(table[request_id]==OUTPUT_ERROR_MSG) sendStatus(OUTPUT_ERROR_MSG, finalFile, clientSocket);
        else if(table[request_id]==PROCESSING) send(clientSocket, "Request is still being processed.", 33,0);

        close(clientSocket);
        return;
    }
    else{
        ssize_t bytessent = send(clientSocket, "Invalid request ID", 18, 0);
            close(clientSocket);
            return ;
    }
}

void* threadFunction(void* arg){
    while(1){
        pthread_mutex_lock(&dequeue_lock);
        while (requests.empty()) 
        {
            pthread_cond_wait(&condition, &dequeue_lock);
        }
        int *clientSocket  = new int;
        *clientSocket = requests.front();
        requests.pop();
        pthread_mutex_unlock(&dequeue_lock);

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead = recv(*clientSocket, buffer, sizeof(buffer), 0);
        buffer[bytesRead]='\0';
        string type(buffer);
        ssize_t byteswrite = send(*clientSocket, "Ready to serve", 14, 0);
        if (strcmp(buffer, "request") == 0) 
        {   
            long long int temp_count;
            pthread_mutex_lock(&counter_lock);
            counter++;
            temp_count=counter;
            pthread_mutex_unlock(&counter_lock);
            requestHandler(temp_count,*clientSocket);
        }
        else
        { 
            sleep(1);
            statusHandler(*clientSocket);

        }
    }

}


int main(int argc, char* argv[]){
    if (argc != 3) 
    {
        cerr << "Usage: " << argv[0] << " <port>" << " <number_of_threads> " << endl;
        return 1;
    }

    makeDirectories();

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error binding socket." << endl;
        return 1;
    }

    if (listen(serverSocket, 1000) == -1) {
        cerr << "Error listening for connections." << endl;
        return 1;
    }

    cout << "Server listening on port " << atoi(argv[1]) << "..." << endl<<flush;
    pthread_mutex_init(&counter_lock, NULL);
    pthread_mutex_init(&enqueue_lock, NULL);
    pthread_mutex_init(&dequeue_lock, NULL);

    int thread_pool_size = atoi(argv[2]);

    pthread_t thread_pool[thread_pool_size];

    for (int i = 0; i < thread_pool_size; i++) {
        pthread_create(&thread_pool[i], nullptr, threadFunction, nullptr);
    }

    while(true){
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            cerr << "Error accepting connection." << endl;
            continue;
        }
        pthread_mutex_lock(&enqueue_lock);
        requests.push(clientSocket);
        // table[counter]=IN_QUEUE;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&enqueue_lock);

    }

    return 0;

}



