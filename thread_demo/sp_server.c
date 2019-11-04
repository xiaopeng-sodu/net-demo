#include "sp_server.h"
#include "sp_epoll.h"

#include <sys/epoll.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>


#define HOST "0.0.0.0"
#define PORT 8889
#define MAX_SOCKET 65535
#define HASH_ID(id)  ((unsigned)(id))%MAX_SOCKET
#define DEFAULT_BUFFER_SIZE 64

typedef (void)(*recv_proc_func)(void *argc); 
typedef (void)(*write_proc_func)(void *argc); 

struct write_buffer {
	char * ptr; 
	void * buffer; 
	int sz; 
	struct write_buffer * next; 
};

struct wb_list {
	struct write_buffer * head; 
	struct write_buffer * tail; 
}

struct socket {
	int id; 
	int sock; 
	struct wb_list high; 
	struct wb_list low; 
	int wb_sz; 
	int size; 
	recv_proc_func recv_func; 
	write_proc_func write_func; 
};

struct socket_server {
	int epfd;    
	int listensock;  
	int allocid; 
	struct socket slot[MAX_SOCKET]; 
}; 


struct socket_server * S = NULL; 

int 
do_listen(){
	int status; 
	int sock; 

	struct addrinfo ai_hints, *ai_list; 
	memset(&ai_hints, 0, sizeof(ai_hints)); 
	ai_hints.ai_flags = AI_PASSIVE; 
	ai_hints.ai_family = AF_INET; 
	ai_hints.ai_socktype = SOCK_STREAM; 
	ai_hints.ai_protocol = IPPROTO_TCP ; 

	char service[16]; 
	sprintf(service, "%d", PORT); 

	status = getaddrinfo(HOST, service, &ai_hints, &ai_list); 
	if(status ! = 0){
		return -1; 
	}

	sock = socket(AF_INET, ai_list->ai_socktype, ai_list->ai_protocol); 
	if (sock < 0){
		goto _failed_fd; 
	}

	status = bind(sock, (struct sockaddr*)ai_list->ai_addr, ai_list->ai_addrlen); 
	if (status ! = 0){
		goto failed; 
	}

	status = listen(sock, 5); 
	if (status != 0){
		goto failed; 
	}

	return sock; 

_failed: 
	close(sock);
_failed_fd: 
	freeaddrinfo( ai_list ); 
	return -1; 
}

void 
create_socket_server(){
	struct socket_server *s = malloc(sizeof(*s)); 
	memset(s, 0, sizeof(*s)); 
	s->epfd = 0; 
	s->listensock = 0; 
	s->allocid = 0; 
}

int
set_reuseaddr(int sock){
	int reuse = 0; 
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1){
		return -1; 
	}
	return 0; 
}

void
clear_wb_list(struct wb_list *wb){
	wb->head = NULL; 
	wb->tail = NULL; 
}

int 
reserve_id(){
	for(;;){
		int id; 
		id = ATOM_INC(&S->allocid); 
		struct socket * s = &S->slot[HASH_ID(id)]; 
		if (s->id == -1){
			s->id = id; 
			return id; 
		}
	}
	return -1; 
}

void 
free_wb_buffer(struct wb_list *wb){
	struct write_buffer * buf = wb->head; 
	while(buf){
		struct write_buffer *tmp = buf->next; 
		free(buf); 
		buf = tmp
	}
	wb->head = NULL; 
	wb->tail = NULL; 
}

void 
force_close(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	int sock = s->sock; 
	sp_del(S->epfd, sock); 
	close(sock);
	free_wb_buffer(&s->high); 
	free_wb_buffer(&s->low); 
}

int 
recv_buffer(int id){
	struct socket * s = &S->slot[HASH_ID(id)]; 
	assert(s); 
	int sock = s->sock; 
	int n; 
	char * buffer = (char *)malloc(sizeof(s->size)); 
	n = read(sock, buffer, s->size); 
	if (n < 0){
		free(buffer); 
		switch(errno){
			case EINTR:
				break; 
			case EAGAIN: 
				break; 
			default: 
				force_close(id); 
				return -1; 
		}
	}

	if (n == 0){
		force_close(sock);
		return 0; 
	}

	
}

void 
init_socket_server(){
	int epfd; 
	int sock; 
	struct socket_server *ss = S; 

	epfd = sp_create(); 

	if (sp_invalid(epfd)){
		fprintf(stderr, "sp_create failed error : %s\n", strerror(errno)); 
		return ; 
	}

	sock = do_listen(); 
	if (sock == -1){
		fprintf(stderr, "do_listen failed error : %s\n", strerror(errno)); 
		return ; 
	}

	set_nonblocking(sock); 
	set_reuseaddr(sock); 

	ss->epfd = epfd; 
	ss->listensock = sock; 

	int i; 
	for(i=0;i<MAX_SOCKET;i++){
		struct socket * s = &ss->slot[i]; 
		s->id = -1; 
		s->sock = -1; 
		s->wb_sz = 0; 
		s->size = DEFAULT_BUFFER_SIZE; 
		clear_wb_list(&s->high); 
		clear_wb_list(&s->low); 
	}

	int id = reserve_id(); 
	struct socket *s = &S->slot[HASH_ID(id)]; 
	s->id = id; 
	s->sock = sock; 
	s->recv_func = recv_buffer; 
	s->write_func = write_buffer; 
}



int
main(int argc, char *argv[]){

	create_socket_server(); 

	return 0; 
}


