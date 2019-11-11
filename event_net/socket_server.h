#ifndef socket_server_h
#define socket_server_h

#include <sys/epoll.h>

#define MAX_EVENTS 50

struct socket_server *S = NULL; 

typedef struct socket_server{
	int epfd; 
	int event_index; 
	const char *host; 
	int port; 
	struct epoll_event evs[MAX_EVENTS]; 
}socket_server; 

void socket_server_create(); 
void socket_server_init(const char *host, int port); 
void socket_server_free(); 


#endif