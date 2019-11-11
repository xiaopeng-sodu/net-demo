#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket_server.h"
#include "sp_epoll.h"
#include "socket.h"
#include "event.h"

struct socket_server *Q = NULL; 

struct socket_server * 
socket_server_init(){
	int epfd; 
	epfd = epoll_create(1024);
	if(sp_invalid(epfd)){
		fprintf(stderr, "sp_create failed\n"); 
		return ; 
	}

	struct socket_server * s = Q; 
	s->epfd = epfd; 
	s->event_index = 0; 
}

void 
socket_server_create(){
	struct socket_server * s = malloc(sizeof(*s)); 
	memset(s, 0, sizeof(*s)); 
	s->epfd = -1; 

	Q = s; 
}

void 
socket_server_free(){
	struct socket_server* s = Q; 
	if(s->epfd > 0){
		close(s->epfd); 
	}
	free(s); 
}