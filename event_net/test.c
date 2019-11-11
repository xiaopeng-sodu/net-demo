#include <stdio.h>

#include "socket_server.h"
#include "listener.h"
#include "sp_epoll.h"
#include "error.h"

#define host "0.0.0.0"
#define port 8888

int 
main(int argc, char *argv[]){

	socket_server_create(); 
	struct socket_server * s = socket_server_init(); 
	set_info("epfd : %d", s->epfd); 
	socket_listen(s->epfd, host, port); 
	epoll_dispatch(s->epfd); 
	socket_server_free(); 

	return 0; 

}