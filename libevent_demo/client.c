#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include <event2/event.h>

#define LOG_MSG_SIZE 256
#define HOST "0.0.0.0"
#define PORT  8888
#define MIN_BUFFER_SIZE 64

union sockaddr_all{
	struct sockaddr s; 
	struct sockaddr_in v4; 
	struct sockaddr_in6 v6; 
}; 

char*
strup(const char *str){
	int len = strlen(str); 
	char *tmp = (char*)malloc(len +1); 
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
		data = strup(msg); 
	}else{
		int max_size = LOG_MSG_SIZE; 
		for(;;){
			max_size *= 2; 
			data = (char *)malloc(max_size); 
			va_list ap ; 
			va_start(ap, fmt); 
			len = vsnprintf(data, max_size, fmt, ap); 
			va_end(ap); 
			if(len < max_size){
				break; 
			}
			free(data); 
		}
	}

	if(len <= 0){
		free(data); 
		return ; 
	}

	printf("%s\n", data); 

	return ; 
}


int 
tcp_connect_server(const char * host, int port){
	set_error("start"); 
	int status; 
	int sock; 
	struct addrinfo ai_hints; 
	struct addrinfo *ai_list; 
	memset(&ai_hints, 0, sizeof(ai_hints)); 

	ai_hints.ai_family = AF_INET; 
	ai_hints.ai_socktype = SOCK_STREAM; 
	ai_hints.ai_protocol = IPPROTO_TCP; 

	char service[16];
	sprintf(service, "%d", port); 
	status = getaddrinfo(host, service, &ai_hints, &ai_list);
	if(status != 0){
		set_error("getaddrinfo status: %d", status); 
		goto _failed; 
	}

	sock = socket(ai_list->ai_family, ai_list->ai_socktype, 0); 
	if(sock < 0){
		set_error("socket status : %d", sock); 
		goto _failed; 
	}

	struct sockaddr_in * s = (struct sockaddr_in*)ai_list->ai_addr; 
	int sin_port = ntohs(s->sin_port); 
	char tmp[32]; 
	inet_ntop(AF_INET, (void*)&s->sin_addr, tmp, sizeof(tmp)); 
	set_error("%s:%d", tmp, sin_port); 

	// union sockaddr_all u;
	// int len = sizeof(u); 
	// if(getsockname(sock, &u.s, &len) == 0){
	// 	void * sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void*)&u.v6.sin6_addr; 
	// 	int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port); 
	// 	char tmp[64]; 
	// 	inet_ntop(AF_INET, sin_addr, tmp, sizeof(tmp));  
	// 	set_error("%s:%d", tmp, sin_port); 
	// }

	// struct sockaddr addr; 
	// memset(&addr, 0, sizeof(addr));
	// int len = sizeof(addr);  
	// if(getsockname(sock, &addr, &len) == 0){
	// 	struct sockaddr_in * s = (struct sockaddr_in*)&addr; 
	// 	int sin_port = ntohs(s->sin_port); 
	// 	char tmp[32]; 
	// 	inet_ntop(AF_INET, (void*)&s->sin_addr, tmp, sizeof(tmp)); 
	// 	set_error("%s:%d", tmp, sin_port);
	// }

	status = connect(sock, ai_list->ai_addr, ai_list->ai_addrlen); 
	if(status != 0){
		set_error("connect status: %d", status); 
		goto _failed_fd; 
	}

	evutil_make_socket_nonblocking(sock); 

	return sock; 

_failed_fd: 
	close(sock); 
_failed: 
	freeaddrinfo(ai_list); 
	return -1;
}

void
socket_read_db(int sock, short events, void *arg){
	char * data = (char *)malloc(MIN_BUFFER_SIZE); 
	int len = read(sock, data, MIN_BUFFER_SIZE); 
	if(len < 0){
		switch(errno){
			case EINTR:
			case EAGAIN: 
				break; 
			default:
				fprintf(stderr, "read _failed: %s\n", strerror(errno)); 
				close(sock); 
				return ; 
		}
	}

	if(len == 0){
		set_error("close sock"); 
		close(sock); 
	}

	set_error("%s", data); 
	free(data); 

	return; 
}

void
cmd_msg_db(int sock, short events, void *arg){
	char *data = (char *)malloc(MIN_BUFFER_SIZE); 
	int n = read(sock, data, MIN_BUFFER_SIZE); 
	if (n < 0){
		switch(errno){
			case EINTR:	
			case EAGAIN: 
				break; 
			default: 
				fprintf(stderr, "read faild: %s\n", strerror(errno)); 
				close(sock); 
				return ; 
		}
	}

	if(n == 0){
		close(sock); 
		return ; 
	}

	int fd = *((int *)arg);

	int len = write(fd, data, n);  
	if (len < 0){
		switch(errno){
			case EINTR:
			case EAGAIN:
				break; 
			default:
				fprintf(stderr, "write _failed: %s\n", strerror(errno)); 
				close(fd); 
				return ; 
		}
	}

	set_error("%s", data); 
	free(data); 
}


int
main(int argc, char *argv[]){

	set_error("client start"); 

	int sock = tcp_connect_server(HOST, PORT); 
	if(sock == -1){
		perror("tcp_connect_server _failed"); 
		return -1; 
	}

	set_error("sock : %d", sock); 

	struct event_base * base = event_base_new(); 

	struct event * ev = event_new(base, sock, EV_READ | EV_PERSIST, socket_read_db, NULL); 

	event_add(ev, NULL); 

	struct event *cmd_ev = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, cmd_msg_db, (void*)&sock); 

	event_add(ev, NULL); 

	event_base_dispatch(base); 

	return 0; 

}