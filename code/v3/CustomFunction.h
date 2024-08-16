void *thread_function(void *arg);
int *enqueue(int *client_sockfd);
void masterFunction(int *pClient);
void *start_function(int *sockfd);
int *dequeue();
int q_size();

