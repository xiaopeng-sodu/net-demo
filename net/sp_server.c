#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <malloc.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "atomic.h"
#include "sp_epoll.h"
#include "sp_error.h"

#define LOG_MSG_SIZE 256
#define HOST "0.0.0.0"
#define PORT 8888
#define MAX_EVENTS 50
#define MAX_SOCKETS 65535

struct write_buffer{
	void * ptr; 
	char * data; 
	struct write_buffer * next; 
};	

struct wb_list {
	struct write_buffer * head;
	struct write_buffer * tail;
};	

struct socket {
	int fd;
	struct wb_list high; 
	struct wb_list low; 
	int wb_size; 
	int size; 
};

struct socket_server {
	int epfd; 
	int event_n;
	int event_index; 
	int listen_sock; 
	int alloc_id; 
	struct event evs[MAX_EVENTS]; 
	struct socket slot[MAX_SOCKETS]; 
};

struct socket_server *S = NULL; 


int max = 0; 

int 
reserve_id(){
	struct socket_server *s =S; 
	int alloc_id = ATOM_INC(&s->alloc_id); 

}

void
socket_server_poll(){
	int i; 
	int n; 
	struct event evs[MAX_EVENTS]; 
	n = sp_wait(evs); 
	setInfo("n : %d", n); 
	for(i=0;i<n;i++){
		int sock = evs[i].sock; 
		int read = evs[i].read; 
		int write = evs[i].write; 

		setInfo("sock : %d, read : %d, write : %d", sock, read, sock); 

		if (sock == S->listen_sock)
		{
			do_accept(); 
		}
		else if (read)
		{
			recv_info(sock); 
			// write_info(sock); 
		}
		else if (write)
		{
			write_info(sock);
		}
		else
		{
			setError("epoll-wait error");
			break; 
		}
	}
}

void 
free_socket_server(int sock){
	sp_del(S->epfd, sock); 
	close(S->epfd); 
	close(sock); 
	free(S); 
}

int 
recv_info(int sock){
	int n; 
	char msg[LOG_MSG_SIZE]; 
	n = read(sock, msg, LOG_MSG_SIZE); 
	if (n < 0){
		switch(errno){
			case EINTR:
				break; 
			case EAGAIN: 
				fprintf(stderr, "recv-info error : %s\n", strerror(errno)); 
				break; 
			default: 
				break; 
		}
	}

	if (n == 0){
		sp_del(S->epfd, sock); 
		close(sock);
		setInfo("close-socket");  
	}
	setInfo("recv, n : %d, msg : %s\n", n, msg); 
}

int 
write_info(int sock){
	int n; 
	char msg[LOG_MSG_SIZE] = "msg from server"; 
	n = write(sock, msg, LOG_MSG_SIZE); 

	return n; 
}

int
do_accept(){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	int len; 
	len = sizeof(addr); 
	int new_sock; 
	new_sock = accept(S->listen_sock, (struct sockaddr*)&addr, &len); 
	if (new_sock <= 0){
		fprintf(stderr, "accept failed, new_sock : %d, error: %s\n", new_sock, strerror(errno)); 
		return -1; 
	}

	set_nonblocking(new_sock); 
	set_reuse(new_sock); 

	struct event ev; 
	ev.sock = new_sock; 
	ev.read = 0; 
	ev.write = 0; 
	ev.ptr = NULL; 
	sp_add(S->epfd, new_sock, (void*)&ev); 

	return new_sock; 
}

int 
sp_wait(struct event *uevs){
	int n; 
	struct epoll_event evs[50]; 
	n = epoll_wait(S->epfd, evs, 50, -1); 
	int i; 
	for (i=0;i<n;i++){
		struct event* ev = (struct event*)evs[i].data.ptr; 
		uevs[i].sock = ev->sock; 
		uevs[i].read = (evs[i].events & EPOLLIN) ? 1: 0; 
		uevs[i].write = (evs[i].events & EPOLLOUT) ? 1 : 0;  
	}
	return n; 
}

int 
do_bind(int sock){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, HOST, &addr.sin_addr); 

	int ret; 
	ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	
	return ret; 
}

int
init_socket_server(){
	int sock; 
	int ret; 
	int epfd; 
	struct socket_server *s = S; 

	epfd = sp_create(); 
	if (epfd <= 0){
		fprintf(stderr, "epoll-create failed epfd : %d, error: %s\n", epfd, strerror(errno)); 
		return -1; 
	}
	s->epfd = epfd; 
	
	sock = socket(AF_INET, SOCK_STREAM , 0); 
	if (sock <= 0){
		fprintf(stderr,"socket failed listen_sock: %d, error: %s\n", sock, strerror(errno)); 
		return -1; 
	}

	set_nonblocking(sock);
	set_reuse(sock); 

	ret = do_bind(sock); 
	if (ret == -1){
		fprintf(stderr, "bind sock failed, ret : %d, error: %s\n", ret, strerror(errno)); 
		return -1; 
	}

	ret = listen(sock, 5); 
	if (ret == -1){
		fprintf(stderr, "listen sock failed, ret : %d, error: %s\n", ret, strerror(errno)); 
		return -1; 
	}

	struct event ev;
	memset(&ev, 0, sizeof(ev)); 
	ev.sock = sock; 
	ev.read = 0; 
	ev.write = 0; 
	ev.ptr = NULL; 

	sp_add(S->epfd, sock, (void*)&ev); 

	s->listen_sock = sock; 
	s->event_n = 0; 
	s->event_index = 0; 

	return ret; 
}

void
create_socket_server(){
	struct socket_server *s = malloc(sizeof(struct socket_server)); 
	memset(s, 0, sizeof(*s)); 
	s->epfd = 0; 
	s->listen_sock = 0;
	s->event_n = 0; 
	s->event_index = 0; 
	s->alloc_id = 0; 
	S = s; 
}

int
main(int argc, char * argv[]){

	int ret; 
	int n; 

	create_socket_server();   /* malloc struct socket_server  */

	ret = init_socket_server();   /* init struct socket_server */
	if (ret == -1){
		setError("init-socket-server failed ret : %d", ret); 
		return -1;
	}

	for(;;)
	{
		socket_server_poll(); 
	}

	// free_socket_server(); 

	return 0;
}
