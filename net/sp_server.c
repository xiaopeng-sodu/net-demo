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

struct event {
	int sock; 
	int read; 
	int write; 
	void * ptr; 
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

int sp_add(int sock, void *ud); 
void sp_del(int sock); 
int sp_wait(struct event * evs); 
int sp_create(); 

int do_bind(int sock); 
int do_accept(); 

void setError(const char *fmt, ...); 
void setInfo(const char * fmt, ...); 

void create_socket_server(); 
int init_socket_server(); 
void socket_server_poll(); 

void set_nonblocking(int sock); 
void set_reuse(int sock); 

int recv_info(int sock); 
int write_info(int sock); 

void free_socket_server(); 

int reserve_id();

int max = 0; 

int
main(int argc, char * argv[]){

	// setInfo("argc: %d", argc); 
	if (argc > 0){
		setInfo("%s", argv[0]); 
	}


	setInfo("EPOLLIN: %d, EPOLLOUT: %d, EPOLLERR: %d, EPOLLRDHUP: %d, EPOLLHUP : %d", \
		EPOLLIN, EPOLLOUT, EPOLLERR, EPOLLRDHUP, EPOLLRDHUP); 

	int ret; 
	int n; 

	create_socket_server();   /* malloc struct socket_server  */

	ret = init_socket_server();   /* init struct socket_server */
	if (ret == -1){
		setError("init-socket-server failed ret : %d", ret); 
		return -1;
	}

	// setInfo("ret : %d, epfd: %d, listen_sock : %d", ret, S->epfd, S->listen_sock); 

	for(;;)
	{
		socket_server_poll(); 
	}

	free_socket_server(); 

	return 0;
}

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
free_socket_server(){
	sp_del(S->epfd); 
	close(S->epfd); 
	close(S->listen_sock); 
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
		sp_del(sock); 
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
	sp_add(new_sock, (void*)&ev); 

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

void
sp_del(int sock){
	epoll_ctl(S->epfd, EPOLL_CTL_DEL, sock, NULL);
}

int 
sp_add(int sock, void *ud){
	struct epoll_event ev;
	ev.events = EPOLLIN; 
	ev.data.ptr = ud; 
	if (epoll_ctl(S->epfd, EPOLL_CTL_ADD, sock, &ev) != 0){
		return -1; 
	}
	return 0;
}

int 
sp_create(){
	return epoll_create(1024); 
}

void 
set_nonblocking(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flags); 
}

void 
set_reuse(int sock){
	int reuse = 1; 
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1){
		fprintf(stderr, "setsockopt SO_REUSE failed"); 
		return;
	}
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

	sp_add(sock, (void*)&ev); 

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

void 
setError(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ;
	va_start(ap, fmt);
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}

void 
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}