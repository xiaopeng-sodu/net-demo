#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include <stdio.h>

#include "listener.h"
#include "socket.h"
#include "socket_server.h"
#include "connectioner.h"
#include "event.h"

#define  BACKLOG 5

int 
do_listen(const char *host, int port){
	int status; 
	int sock; 
	struct addrinfo ai_hints ; 
	struct addrinfo *ai_list; 
	memset(&ai_hints, 0, sizeof(ai_hints));

	ai_hints.ai_family = AF_INET; 
	ai_hints.ai_socktype = SOCK_STREAM; 

	char service[16]; 
	sprintf(service, "%d", port); 

	status = getaddrinfo(host, service, &ai_hints, &ai_list); 
	if(status == -1){
		goto _failed; 
	}

	sock = socket(ai_list->ai_family, ai_list->ai_socktype, 0); 
	if(sock < 0){
		goto _failed; 
	}

	set_reuseaddr(sock); 
	set_nonblocking(sock); 

	status = bind(sock, ai_list->ai_addr, ai_list->ai_addrlen); 
	if(status == -1){
		goto _failed_sock; 
	}

	status = listen(sock, BACKLOG); 
	if(status == -1){
		goto _failed_sock; 
	}

	freeaddrinfo(ai_list); 

	return sock; 

_failed_sock: 
	close(sock); 
_failed: 
	freeaddrinfo(ai_list); 
	return -1; 
}

int 
socket_listen(){
	const char *host = S->host; 
	int port = S->port; 
	int sock = do_listen(host, port); 
	if(sock == -1){
		fprintf(stderr, "socket_listen _failed\n"); 
		return -1; 
	}

	create_event(sock, accept_sock, NULL); // new listen event

}