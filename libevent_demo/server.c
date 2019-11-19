#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdlib.h>


#include <event2/event.h>

#define HOST "0.0.0.0"
#define PORT 8888
#define BACKLOG 5
#define LOG_MSG_SIZE 256
#define MIN_BUFFER_SIZE 64

char *
malloc_cpy(const char *str){
	int len = strlen(str); 
	char * tmp = malloc(len+1); 
	memcpy(tmp, str, len +1); 
	return tmp; 
}

void
set_error(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	int len = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 

	char * data; 
	if(len > 0 && len < LOG_MSG_SIZE){
		data = malloc_cpy(msg); 
	}else{
		int max_size = LOG_MSG_SIZE; 
		for(;;){
			max_size *= 2; 
			data = malloc(max_size); 
			va_list ap ; 
			va_start(ap, fmt); 
			len = vsnprintf(data, max_size, fmt, ap); 
			if(len < max_size){
				break; 
			}
			free(data); 
		}
	}

	if(len < 0){
		free(data); 
		perror("vsnprintf failed"); 
		return; 
	}

	printf("%s\n", data); 
	free(data); 
	return; 
}

int
tcp_server_init(int port, int backlog){

	int status; 
	int listen_sock; 

	listen_sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (listen_sock <= 0){
		return -1; 
	}

	evutil_make_listen_socket_reuseable(listen_sock); 

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(port); 
	inet_aton(HOST, &addr.sin_addr); 

	status = bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if(status != 0){
		goto failed; 
	}

	status = listen(listen_sock, backlog); 
	if(status != 0){
		goto failed; 
	}

	evutil_make_socket_nonblocking(listen_sock); 

	return listen_sock; 

failed: 
	close(listen_sock); 
	return -1; 

}

void
socket_read_cb(int client_sock, short events, void *arg){
	struct event * ev = (struct event*)arg; 
	int len; 
	char *data = malloc(MIN_BUFFER_SIZE); 
	len = read(client_sock, data, MIN_BUFFER_SIZE); 
	if(len < 0){
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN:
				break; 
			default:
				fprintf(stderr, "read socket failed: %s\n", strerror(errno)); 
				close(client_sock); 
				return; 
		}
	}

	if (len == 0){
		close(client_sock); 
		return; 
	}

	set_error("%s", data); 
	free(data); 

	return ;
}



void 
accept_cb(int sock , short events, void *arg){
	int client_sock; 
	int len; 

	struct sockaddr_in client_addr; 
	memset(&client_addr, 0, sizeof(client_addr)); 
	len = sizeof(client_addr); 

	client_sock = accept(sock, (struct sockaddr*)&client_addr, &len); 
	if(client_sock < 0){
		return; 
	}

	struct sockaddr addr; 
	memset(&addr, 0, sizeof(addr)); 
	len = sizeof(addr);
	if (getpeername(client_sock, &addr, &len) == 0){
		struct sockaddr_in * s = (struct sockaddr_in*)&addr; 
		int sin_port = ntohs(s->sin_port); 
		char tmp[32]; 
		inet_ntop(AF_INET, (void*)&s->sin_addr, tmp, sizeof(tmp)); 
		set_error("%s:%d", tmp, sin_port); 
	}


	set_error("client_sock : %d", client_sock); 

	evutil_make_socket_nonblocking(client_sock); 

	struct event * ev = event_new(NULL, -1, 0, NULL, NULL); 

	struct event_base * base = (struct event_base*)arg; 

	event_assign(ev, base, client_sock, EV_READ | EV_PERSIST, socket_read_cb, (void*)ev); 

	event_add(ev, NULL); 
}


int
main(int argc, char *argv[]){

	int listen_sock = tcp_server_init(PORT, BACKLOG); 

	set_error("listen_sock: %d", listen_sock); 

	struct event_base * base = event_base_new(); 

	struct event * ev_listen = event_new(base, listen_sock, EV_READ | EV_PERSIST, accept_cb, base); 

	event_add(ev_listen, NULL); 

	event_base_dispatch(base); 

	return -1; 

}