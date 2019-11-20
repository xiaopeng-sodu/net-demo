#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>

#include <event2/event.h>

#define HOST "0.0.0.0"
#define PORT 8888
#define BACKLOG 5
#define MIN_BUFFER_SIZE 64
#define LOG_MSG_SIZE 256

char *
str_up(const char * str){
	int len = strlen(str);
	char * tmp = malloc(len + 1); 
	memcpy(tmp, str, len + 1); 
	return tmp; 
}

void
set_info(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	int n = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 

	char * data = NULL; 

	if(n > 0 && n < LOG_MSG_SIZE){
		data = str_up(msg); 
	}else{
		int max_size = LOG_MSG_SIZE; 
		for(;;){
			max_size *= 2; 
			data = (char *)malloc(max_size); 
			va_list ap ; 
			va_start(ap, fmt); 
			n = vsnprintf(data, max_size, fmt, ap); 
			va_end(ap); 
			if(n < max_size){
				break; 
			}
			free(data); 
		}
	}

	if(n < 0){
		free(data); 
	}

	printf("%s\n", data); 
	free(data); 

}

int 
tcp_server_init(){
	int status; 
	int listen_sock; 
	struct addrinfo ai_hints; 
	struct addrinfo *ai_list; 

	memset(&ai_hints, 0, sizeof(ai_hints)); 

	ai_hints.ai_family = AF_INET; 
	ai_hints.ai_socktype = SOCK_STREAM; 
	ai_hints.ai_protocol = IPPROTO_TCP;

	char service [64]; 
	sprintf(service, "%d", PORT); 

	status = getaddrinfo(HOST, service, &ai_hints, &ai_list); 
	if(status != 0){
		goto _failed; 
	}

	listen_sock = socket(AF_INET, ai_list->ai_socktype, 0); 
	if(listen_sock < 0){
		goto _failed; 
	}

	status = bind(listen_sock, ai_list->ai_addr, ai_list->ai_addrlen); 
	if(status != 0){
		goto _failed_fd; 
	}

	status = listen(listen_sock, BACKLOG); 
	if(status != 0){
		goto _failed_fd;
	}

	evutil_make_socket_nonblocking(listen_sock); 
	evutil_make_listen_socket_reuseable(listen_sock); 

	return listen_sock; 

_failed_fd: 
	close(listen_sock); 
_failed: 
	freeaddrinfo(ai_list); 
	return -1; 
}

void
socket_read_cb(int client_sock, short events, void *arg){
	int n; 
	char *data = (char *)malloc(MIN_BUFFER_SIZE); 
	memset(data, 0, MIN_BUFFER_SIZE); 
	n = read(client_sock, data, MIN_BUFFER_SIZE); 
	if(n < 0){
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN: 
				break; 
			default: 
				fprintf(stderr, "read _failed: %s\n", strerror(errno)); 
				close(client_sock); 
				return ; 
		}
	}

	if(n == 0){
		printf("close socket\n"); 
		close(client_sock); 
		return;  
	}

	set_info("%s", data); 
	free(data); 

	char msg[LOG_MSG_SIZE] = "msg from server"; 
	n = write(client_sock, msg, strlen(msg)-1); 
	if(n < 0){
		switch(errno){
			case EINTR:
			case EAGAIN:
			default:
				close(client_sock); 
				return; 
		}
	}

}

void
accept_cb(int listen_sock, short events, void *arg){

	struct event_base * base = (struct event_base*)arg; 

	int client_sock; 
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	int len = sizeof(addr); 

	client_sock = accept(listen_sock, (struct sockaddr*)&addr, &len); 
	if(client_sock < 0){
		fprintf(stderr, "accept _failed"); 
		return; 
	}

	set_info("client_sock : %d", client_sock); 

	// struct sockaddr_in *s = (struct sockaddr_in*)&addr; 
	// void * sin_addr = (void*)&s->sin_addr;
	// char tmp[32]; 
	// inet_ntop(AF_INET, sin_addr, tmp, sizeof(tmp));  
	// short sin_port = ntohs(s->sin_port); 
	// set_info("client_sock: %s:%d:%d", tmp, sin_port, s->sin_port); 

	// struct sockaddr_in taddr; 
	// len = sizeof(taddr); 
	// getpeername(client_sock, (struct sockaddr*)&taddr, &len); 
	// void *k_addr = (void*)&taddr.sin_addr; 
	// char ktmp[32]; 
	// memset(ktmp, 0, sizeof(ktmp)); 
	// inet_ntop(AF_INET, k_addr, ktmp, sizeof(ktmp)); 
	// short k_port = ntohs(taddr.sin_port); 
	// set_info("k_addr: %s: %d", ktmp, k_port); 


	evutil_make_socket_nonblocking(client_sock); 

	struct event * client_ev = event_new(base, client_sock, EV_READ | EV_PERSIST, socket_read_cb, NULL); 

	event_add(client_ev, NULL); 
}

int 
main(int argc, char *argv[]){

	int listen_sock ;

	listen_sock = tcp_server_init(); 

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	int len = sizeof(addr); 
	getsockname(listen_sock, (struct sockaddr*)&addr, &len); 

	void *sin_addr = &addr.sin_addr;
	char tmp[32]; 
	inet_ntop(AF_INET, sin_addr, tmp, sizeof(tmp)); 
	short sin_port = ntohs(addr.sin_port); 

	set_info("listen_sock_addr: %s: %d", tmp, sin_port); 

	set_info("listen_sock : %d", listen_sock); 	

	struct event_base * base = event_base_new(); 

	struct event * listen_ev = event_new(base, listen_sock, EV_READ | EV_PERSIST, accept_cb, (void*)base); 

	event_add(listen_ev, NULL); 

	event_base_dispatch(base); 

	event_base_free(base); 

	return 0; 
}