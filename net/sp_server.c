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

#define HASH_ID(id)  (((unsigned)id) % MAX_SOCKETS)

typedef int(* read_proc_func)(int sock); 
typedef int(* write_pro_func)(int sock); 


struct write_buffer{
	void * buffer; 
	char * ptr; 
	int sz; 
	struct write_buffer * next; 
};	

struct wb_list {
	struct write_buffer * head;
	struct write_buffer * tail;
};	

struct socket {
	int fd;
	int id; 
	struct wb_list high; 
	struct wb_list low; 
	int wb_size; 
	int size; 
	read_proc_func  read_func; 
	write_pro_func  write_func; 
};

struct socket_server {
	int epfd; 
	int event_index; 
	int event_n;
	int listen_sock; 
	int alloc_id; 
	struct event evs[MAX_EVENTS]; 
	struct socket slot[MAX_SOCKETS]; 
};

struct socket_server *S = NULL; 


int 
reserve_id(){
	struct socket_server *ss =S; 
	int i; 
	for (i=0;i<MAX_SOCKETS;i++){
		int alloc_id = ATOM_INC(&ss->alloc_id); 

		setInfo("alloc_id : %d", alloc_id); 

		struct socket * s = &ss->slot[HASH_ID(alloc_id)]; 
		if (s->id == -1){
			s->id = alloc_id; 
			return alloc_id; 
		}
	}
	return -1;
}

void 
add_sock_epoll(int id){
	struct event ev; 
	memset(&ev, 0, sizeof(ev)); 

	struct socket * s = &S->slot[HASH_ID(id)]; 
	assert(s->id != 0 && s->fd != 0); 

	ev.sock = s->fd; 
	ev.ptr = s; 
	ev.read = 0; 
	ev.write = 0; 

	sp_add(S->epfd, s->fd, &ev); 
}


void
socket_server_poll(){
	int i; 
	int n; 
	struct event evs[MAX_EVENTS]; 
	n = sp_wait(S->epfd, evs); 
	setInfo("n : %d", n); 
	for(i=0;i<n;i++){
		int sock = evs[i].sock; 
		int read = evs[i].read; 
		int write = evs[i].write; 
		struct socket * s = (struct socket*)evs[i].ptr; 

		setInfo("sock : %d, read : %d, write : %d", sock, read, sock); 

		if (sock == S->listen_sock)
		{
			s->read_func(sock); 
		}
		else if (read)
		{
			s->read_func(sock); 
			// s->write_func(sock); 
		}
		else if (write)
		{
			s->write_func(sock); 
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
	sp_del(S->epfd, S->listen_sock); 
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
	int new_sock; 
	len = sizeof(addr); 
	new_sock = accept(S->listen_sock, (struct sockaddr*)&addr, &len); 
	if (new_sock <= 0){
		fprintf(stderr, "accept failed, new_sock : %d, error: %s\n", new_sock, strerror(errno)); 
		return -1; 
	}

	set_nonblocking(new_sock); 
	set_reuse(new_sock); 

	int id = reserve_id(); 
	if (id == -1){
		fprintf(stderr, "reserve_id failed id: %d, error : %s\n", id, strerror(errno)); 
		return -1; 
	}

	struct socket * s = &S->slot[HASH_ID(id)]; 
	s->fd = new_sock; 
	s->id  = id; 
	s->read_func = recv_info; 

	add_sock_epoll(id); 

	return new_sock; 
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

void
clear_wb_list(struct wb_list * wb){
	wb->head = NULL; 
	wb->tail = NULL; 
}

int
do_listen(){
	int sock;
	int ret; 
	struct socket_server *ss = S; 

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

	int id = reserve_id(); 
	if (id == -1){
		fprintf(stderr, "reserve_id failed, id : %d", id); 
		return -1; 
	}

	struct socket * s = &ss->slot[id]; 
	s->fd = sock; 
	s->read_func = do_accept; 

	add_sock_epoll(id); 

	return sock; 
}

int
init_socket_server(){
	int epfd; 
	struct socket_server *ss = S; 

	epfd = sp_create(); 
	if (sp_invalid(epfd)){
		fprintf(stderr, "epoll-create failed epfd : %d, error: %s\n", epfd, strerror(errno)); 
		return -1; 
	}

	ss->epfd = epfd; 
	ss->event_n = 0; 
	ss->event_index = 0; 
	ss->alloc_id = 0; 

	int i; 
	for(i=0;i<MAX_SOCKETS;i++){
		struct socket * s = &ss->slot[HASH_ID(i)]; 
		s->id = -1; 
		s->wb_size = 0; 
		clear_wb_list(&s->high);
		clear_wb_list(&s->low); 
	}

	int sock; 
	sock = do_listen(); 
	if (sock <= 0){
		fprintf(stderr, "do_listen failed, sock : %d", sock); 
		return -1; 
	}

	ss->listen_sock = sock; 

	return 1; 
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

	create_socket_server();   /* malloc struct socket_server  */

	init_socket_server(); 

	for(;;)
	{
		socket_server_poll(); 
	}

	free_socket_server(); 

	return 0;
}
