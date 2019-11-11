#ifndef listener_h
#define listener_h

int socket_listen(int epfd, const char *host, int port); 
int accept_sock(int epfd, int sock); 

#endif