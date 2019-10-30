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
#include <pthread.h>

#include "atomic.h"
#include "sp_epoll.h"
#include "sp_error.h"
#include "sp_queue.h"

#define LOG_MSG_SIZE 256
#define HOST "0.0.0.0"
#define PORT 8888
#define MAX_EVENTS 50
#define MAX_SOCKETS 65535
#define MIN_BUFF_SIZE 256

#define HASH_ID(id)  (((unsigned)id) % MAX_SOCKETS)

typedef int(* read_proc_func)(int id); 
typedef int(* write_pro_func)(int id); 

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
	struct message_queue * queue; 
};

struct socket_server {
	int epfd; 
	int listen_sock; 
	int alloc_id; 
	struct socket slot[MAX_SOCKETS]; 
};

struct socket_server *S = NULL; 

int 
reserve_id(){
	struct socket_server *ss =S; 
	int i; 
	for (i=0;i<MAX_SOCKETS;i++){
		int alloc_id = ATOM_INC(&ss->alloc_id); 

		// setInfo("alloc_id : %d", alloc_id); 

		struct socket * s = &ss->slot[HASH_ID(alloc_id)]; 
		if (s->id == -1){
			s->id = alloc_id; 
			return alloc_id; 
		}
	}
	return -1;
}

int
sock_epoll(int id, struct event * ev){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	assert(s->id != 0 && s->fd != 0); 

	ev->ptr = s; 
	ev->read = 0; 
	ev->write = 0; 

	return s->fd; 
}

void 
add_sock_epoll(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	assert(s->id != 0 && s->fd != 0); 

	sp_add(S->epfd, s->fd , (void*)s); 
}

int 
socket_server_poll(){
	int i; 
	int n; 

	struct epoll_event evs[MAX_EVENTS]; 
	n = epoll_wait(S->epfd, evs, 50, -1); 

	for(i=0;i<n;i++){
		struct socket * s = (struct socket*)evs[i].data.ptr; 
		int id = s->id; 
		int sock = s->fd; 
		int events = evs[i].events; 

		if (sock == S->listen_sock)
		{
			s->read_func(id); 
		}
		else if (events & EPOLLIN)
		{
			int m = s->read_func(id); 

			if (m > 0){
				s->write_func(id); 
			}
		}
		else if (events & EPOLLOUT)
		{
			s->write_func(id); 
		}
		else
		{
			fprintf(stderr, "error : %s", strerror(errno)); 
			setError("epoll-wait error");
			return -1; 
		}
	}
	return 1; 
}

void 
free_socket_server(){
	sp_del(S->epfd, S->listen_sock); 
	close(S->epfd); 
	close(S->listen_sock); 
	free(S); 
}

void 
free_wb_list(struct wb_list *wb){
	struct write_buffer * buf = wb->head; 
	while(buf){
		struct write_buffer * tmp = buf->next; 
		free(buf); 
		buf = tmp; 
	}
	wb->head = NULL; 
	wb->tail = NULL; 
}

void 
release_socket(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	s->id = -1; 
	s->fd = 0; 
	s->wb_size = 0; 
	free_wb_list(&s->high); 
	free_wb_list(&s->low); 
}

void 
force_close(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	int sock = s->fd; 
	sp_del(S->epfd, sock); 
	close(sock); 
	release_socket(id); 
}

int 
recv_info(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	assert(s->id > 0 && s->fd > 0); 
	int sock = s->fd; 
	int sz = s->size; 
	char * buffer = malloc(sz); 
	int n; 
	n = read(sock, buffer, sz); 
	if (n < 0){
		free(buffer);
		switch(errno){
			case EINTR:
				break; 
			case EAGAIN: 
				fprintf(stderr, "recv-info error : %s\n", strerror(errno)); 
				break; 
			default: 
				fprintf(stderr,"read default , error, close"); 
				force_close(id);
				return -1;  
		}
		return -1; 
	}

	if (n == 0){
		free(buffer); 
		force_close(id);
		setInfo("recv - close , n : %d", n);
		return n; 
	}

	if (n == sz){
		s->size *= 2; 
	}
	else if (sz > MIN_BUFF_SIZE && sz > 2*n){
		s->size /= 2; 
	}

	setInfo("recv, n : %d, buffer : %s", n, buffer); 

	struct skynet_message message; 
	message.ptr = buffer; 
	sp_mq_push(s->queue, &message); 

	int len = sp_mq_length(s->queue); 
	setInfo("mq len : %d", len); 

	// free(buffer); 
	return n; 
}

int 
write_info(int id){
	struct socket * s= &S->slot[HASH_ID(id)]; 
	assert(s->id > 0 && s->fd > 0); 
	int sock = s->fd; 
	int sz = s->size; 
	char *buffer = malloc( sz ); 
	memset(buffer, 0, sz); 
	strcpy(buffer, "msg-from-server"); 
	int n; 
	n = write(sock, buffer, sz); 
	if (n < 0){
		free(buffer); 
		switch(errno){
			case EINTR: 
			case EAGAIN:
				n = 0; 
				break; 
			default: 
				force_close(id);
				break; 
		}
		return -1; 
	}

	free(buffer); 
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

	int id ;
	id = reserve_id(); 
	if (id == -1){
		fprintf(stderr, "reserve_id failed id: %d, error : %s\n", id, strerror(errno)); 
		return -1; 
	}

	struct socket * s = &S->slot[HASH_ID(id)]; 
	s->fd = new_sock; 
	s->id  = id; 
	s->read_func = recv_info; 
	s->write_func = write_info;

	add_sock_epoll(id); 

	struct message_queue * queue = create_message_queue(id); 
	s->queue = queue;

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
	ss->alloc_id = 0; 

	int i; 
	for(i=0;i<MAX_SOCKETS;i++){
		struct socket * s = &ss->slot[HASH_ID(i)]; 
		s->id = -1; 
		s->wb_size = 0; 
		s->size = MIN_BUFF_SIZE;
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
	s->alloc_id = 0; 
	S = s; 
}

void *
thread_socket_func(void * p){
	for(;;)
	{
		int type; 

		type = socket_server_poll(); 

		if (type == -1){
			break; 
		}
	}
}

int
main(int argc, char * argv[]){

	init_global_queue(); 

	create_socket_server();   /* malloc struct socket_server  */

	init_socket_server(); 

	for(;;)
	{
		int type; 

		type = socket_server_poll(); 

		if (type == -1){
			break; 
		}
	}

	// pthread_t tid[2]; 

	// pthread_create(&tid[0], NULL, thread_socket_func, NULL); 

	// pthread_join(tid[0], NULL); 

	free_socket_server(); 

	return 0;
}
