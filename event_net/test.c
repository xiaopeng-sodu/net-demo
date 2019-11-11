#include <stdio.h>

#include "socket_server.h"
#include "listener.h"
#include "epoll.h"

#define host "0.0.0.0"
#define port 8888

int 
main(int argc, char *argv[]){

	socket_server_create(); 
	socket_server_init(host, port); 
	epoll_dispatch(); 
	socket_listen(); 

	return 0; 

}