#ifndef socket_server_h
#define socket_server_h

#include <sys/epoll.h>
#include "sp_epoll.h"

typedef struct socket_server{
	int epfd; 
	int event_index; 
	struct epoll_event evs[50]; 
}socket_server; 

void socket_server_create(); 
struct socket_server * socket_server_init(); 
void socket_server_free(); 


#endif