#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket_server.h"
#include "epoll.h"
#include "socket.h"
#include "event.h"

void 
socket_server_init(const char *host, int port){
	int epfd; 
	epfd = sp_create(); 
	if(sp_invalid(epfd)){
		fprintf(stderr, "sp_create failed\n"); 
		return ; 
	}

	struct socket_server * s= S; 
	s->epfd =epfd; 
	s->event_index = 0; 
	s->host = host; 
	s->port = port; 
}

void 
socket_server_create(const char *host, int port){
	struct socket_server * s = malloc(sizeof(*s)); 
	memset(s, 0, sizeof(*s)); 
	s->epfd = -1; 

	S = s; 
}

void 
socket_server_free(){
	struct socket_server* s = S; 
	if(s->epfd > 0){
		close(s->epfd); 
	}
	free(s); 
}