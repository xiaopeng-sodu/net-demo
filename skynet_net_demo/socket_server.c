#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <netinet/tcp.h>

#include "socket_server.h"
#include "socket_epoll.h"
#include "skynet_error.h"
#include "atomic.h"

#define MAX_INFO 128
#define MAX_EVENTS 50
#define MAX_SOCKET_P 16
#define MAX_SOCKET (1 << MAX_SOCKET_P)
#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2 
#define SOCKET_TYPE_LISTEN  3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_BIND 6
#define SOCKET_TYPE_HALFCLOSE 7
#define SOCKET_TYPE_CLOSE 8
#define SOCKET_TYPE_PACCEPT 9

#define MIN_BUFFER_SIZE 64

#define UDP_ADDRES_SIZE 19

#define PRIORITY_LOW 0
#define PRIORITY_HIGH 1

#define HASH_ID(id)  (((unsigned)id)%MAX_SOCKET)


typedef struct write_buffer{
	char * ptr; 
	void * buffer; 
	int size; 
	struct write_buffer * next; 
	bool userobject; 
	char udp_address[UDP_ADDRES_SIZE]; 
}write_buffer; 

#define SIZEOF_TCPBUFFER (offsetof(struct write_buffer, udp_address[0]))
#define SIZEOF_UDPBUFFER (sizeof(struct write_buffer))

typedef struct wb_list {
	struct write_buffer * head; 
	struct write_buffer * tail; 
}wb_list; 

struct socket {
	int opaque; 
	int fd; 
	int id; 
	int protocol; 
	int type; 
	int wb_size; 
	struct wb_list high; 
	struct wb_list low; 
	union {
		int sz; 
		char udp_address[256]; 
	}p; 
} ; 

struct socket_server {
	int epfd; 
	int event_index; 
	int event_n; 
	int alloc_id; 
	int check_ctrl; 
	int recv_fd; 
	int write_fd; 
	char buffer[MAX_INFO]; 
	struct event evs[MAX_EVENTS]; 
	struct socket slot[MAX_SOCKET];
	fd_set rdfs; 
};

struct request_listen{
	int id; 
	int fd;
	int opaque; 
	char host[1]; 
}; 	
struct request_start{
	int id; 
	int opaque; 
}; 
struct request_send{
	int id; 
	int sz; 
	char *buffer; 
};
struct request_close{
	int id; 
	int shutdown; 
	int opaque; 
}; 
struct request_open{
	int id; 
	int opaque; 
	int port; 
	char host[1]; 
};
struct request_setopt{
	int id; 
	int what; 
	int value; 
};
	

struct request_package{
	char header[8]; 
	union {
		char buffer[256];
		struct request_listen listen; 
		struct request_start start; 
		struct request_send send; 
		struct request_close close; 
		struct request_open open; 
		struct request_setopt setopt; 
	}u; 
	char dummy[256];
};

union sockaddr_all {
	struct sockaddr s; 
	struct sockaddr_in v4; 
	struct sockaddr_in6 v6; 
}; 

struct send_object{
	void * buffer; 
	int sz; 
	void (*free_func)(void *);
};

static inline void
free_func(void *buffer){
	free(buffer); 
}

#define FREE free_func

static inline bool 
send_object_init(struct socket_server *ss, struct send_object * so, void *object , int sz){
	so->buffer = object; 
	so->sz = sz; 
	so->free_func = FREE; 
	return true;
}

void 
clear_wb_list(struct wb_list * wb){
	wb->head = NULL; 
	wb->tail = NULL; 
}

struct socket_server *
socket_server_create(){
	int epfd; 
	int fd[2]; 

	epfd = sp_create(); 
	if(sp_invalid(epfd)){
		fprintf(stderr, "sp_create failed\n"); 
		return NULL;  
	}

	if(pipe(fd)){
		fprintf(stderr, "pipe create failed\n"); 
		close(epfd); 
		return  NULL; 
	}

	if (sp_add(epfd, fd[0], NULL)){
		fprintf(stderr, "sp_add -fd[0] failed\n"); 
		close(epfd); 
		close(fd[0]); 
		close(fd[1]); 
		return NULL; 
	}

	struct socket_server * ss = malloc(sizeof(*ss)); 
	memset(ss, 0, sizeof(*ss)); 

	ss->epfd = epfd; 
	ss->recv_fd = fd[0]; 
	ss->write_fd = fd[1]; 
	ss->event_index = 0; 
	ss->event_n = 0; 
	ss->alloc_id = 0; 
	ss->check_ctrl = 1; 

	int i; 
	for(i=0;i<MAX_SOCKET;i++){
		struct socket *s = &ss->slot[i]; 
		s->type = SOCKET_TYPE_INVALID; 
		s->id = -1; 
		s->fd = -1; 
		s->wb_size = 0; 
		clear_wb_list(&s->high); 
		clear_wb_list(&s->low); 
	}

	FD_ZERO(&ss->rdfs); 
	assert(ss->recv_fd < FD_SETSIZE); 

	return ss; 
}

static int 
reserve_id(struct socket_server *ss){
	int i; 
	for(i=0;i<MAX_SOCKET;i++){
		int id = ATOM_INC(&ss->alloc_id); 
		if(id < 0){
			id 	= ATOM_AND(&ss->alloc_id, 0x7fffffff);
		}
		struct socket * s = &ss->slot[HASH_ID(id)];
		if (s->type == SOCKET_TYPE_INVALID){
			if(ATOM_CAS(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)){
				s->id = id; 
				s->fd = -1; 
				return id;
			}else{
				--i; 
			}
		} 
	}
	return -1; 
}

static void
send_request(struct socket_server *ss, struct request_package * package, int type, int len){
	package->header[6] = (char)type;
	package->header[7] = (char)len; 

	for(;;){
		int n = write(ss->write_fd, &package->header[6], len+2); 
		if (n < 0){
			if (errno != EINTR){
				fprintf(stderr, "socket_server send_request failed: %s\n", strerror(errno));
			}
			continue;
		}
		assert(n == len+2); 
		return ; 
	}
}

void
socket_test(struct socket_server *s){
	skynet_error("epfd : %d, recv_fd : %d, write_fd: %d, size: %d", s->epfd, s->recv_fd, s->write_fd, sizeof(*s)); 
}

static int 
has_cmd(struct socket_server *ss){
	struct timeval tv = {0, 0}; 

	FD_SET(ss->recv_fd, &ss->rdfs); 

	int retval = select(ss->recv_fd+1, &ss->rdfs, NULL, NULL, &tv); 
	if(retval == 1){
		return 1; 
	}
	return 0; 
}

static void
block_readpipe(int sock, void *buffer, int sz){
	for(;;){
		int n = read(sock, buffer, sz); 
		if(n < 0){
			if (errno == EINTR){
				continue; 
			}
			fprintf(stderr, "block_readpipe failed: %s\n", strerror(errno)); 
			return ; 
		}
		assert(n == sz); 
		return; 
	}
}

struct socket *
new_fd(struct socket_server *ss, int id, int fd, int opaque, int protocol, bool add){
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	assert(s->type == SOCKET_TYPE_RESERVE); 

	if (add){
		if (sp_add(ss->epfd, fd, s)){
			s->type = SOCKET_TYPE_INVALID; 
			return NULL; 
		}
	}

	s->id = id; 
	s->fd = fd; 
	s->wb_size = 0; 
	s->p.sz = MIN_BUFFER_SIZE; 
	s->opaque = opaque; 
	s->protocol = protocol; 
	clear_wb_list(&s->high); 
	clear_wb_list(&s->low); 
	return s; 
}

static int 
listen_socket(struct socket_server *ss, struct request_listen *listen, struct socket_message * result){
	int id = listen->id; 
	int fd = listen->fd; 
	skynet_error("listen_socket, id: %d, fd : %d", id, fd); 
	struct socket *s = new_fd(ss, id, fd, listen->opaque, IPPROTO_TCP, false); 
	if (s == NULL){
		goto _failed; 
	}

	s->type = SOCKET_TYPE_PLISTEN; 
	return -1; 

_failed: 
	close(fd); 
	result->id = id; 
	result->ud = 0; 
	result->opaque = listen->opaque; 
	result->data = "listen socket faield"; 
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID; 

	return SOCKET_ERROR; 
}

static void
write_buffer_free(struct write_buffer* wb){
	free(wb->buffer); 
	free(wb); 
}

static void
free_wb_list(struct socket_server *ss, struct wb_list * list){
	struct write_buffer * wb = list->head; 
	while(wb){
		struct write_buffer *tmp = wb; 
		wb = wb->next; 
		write_buffer_free(tmp); 
	}
	list->head = NULL; 
	list->tail = NULL; 
}

static void
force_close(struct socket_server *ss, struct socket *s, struct socket_message * result){
	result->id = s->id; 
	result->opaque = s->opaque; 
	result->ud = 0; 
	result->data = NULL; 
	if(s->type == SOCKET_TYPE_INVALID || s->type == SOCKET_TYPE_RESERVE){
		return ; 
	}

	free_wb_list(ss, &s->high); 
	free_wb_list(ss, &s->low); 

	if(s->type != SOCKET_TYPE_PACCEPT && s->type != SOCKET_TYPE_PLISTEN){
		sp_del(ss->epfd, s->fd); 
	}

	if(s->type != SOCKET_TYPE_BIND){
		if(close(s->fd) < 0){
			perror("close socket : "); 
		}
	}

	s->type = SOCKET_TYPE_INVALID; 

}

static int 
start_socket(struct socket_server *ss, struct request_start *start, struct socket_message *result){
	int id = start->id; 
	int opaque = start->opaque; 
	result->id = id; 
	result->opaque = opaque; 
	result->ud = 0; 
	result->data = NULL; 
	struct socket *s = &ss->slot[HASH_ID(id)]; 
	if (s->type == SOCKET_TYPE_INVALID || s->id != id || s->type == SOCKET_TYPE_RESERVE){
		result->data = "invalid socket"; 
		return SOCKET_ERROR; 
	}

	if(s->type == SOCKET_TYPE_PLISTEN || s->type == SOCKET_TYPE_PACCEPT){
		if(sp_add(ss->epfd, s->fd, s)){
			force_close(ss, s, result); 
			result->data = strerror(errno); 
			return SOCKET_ERROR; 
		}

		s->type = (s->type == SOCKET_TYPE_PACCEPT) ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN; 
		s->opaque = opaque; 
		result->data = "start"; 
		return SOCKET_OPEN; 
	}else if(s->type == SOCKET_TYPE_CONNECTED){
		s->opaque = start->opaque; 
		result->data = "transfer";
		return SOCKET_OPEN; 
	}

	return -1; 
}

static inline int 
send_buffer_empty(struct socket *s){
	return (s->high.head == NULL && s->low.head == NULL); 
}

static struct write_buffer * 
append_sendbuffer_(struct socket_server *ss, struct wb_list *s, struct request_send * send, int size, int n){
	struct write_buffer *buffer = malloc(size); 
	struct send_object so; 
	buffer->userobject = send_object_init(ss, &so, send->buffer, send->sz); 
	buffer->ptr = (char*)so.buffer + n; 
	buffer->size = so.sz - n; 
	buffer->buffer = send->buffer; 
	buffer->next = NULL; 
	if (s->head == NULL){
		s->head = s->tail = buffer; 
	}else{
		assert(s->tail); 
		assert(s->tail->next == NULL); 
		s->tail->next = buffer;
		s->tail = s->tail->next; 
	}
	return buffer; 
}

static inline void
append_sendbuffer(struct socket_server *ss, struct socket *s, struct request_send * send , int n ){
	struct write_buffer * buf = append_sendbuffer_(ss, &s->high, send, SIZEOF_TCPBUFFER, n); 
	s->wb_size += buf->size; 
}


static inline void
append_sendbuffer_low(struct socket_server *ss, struct socket *s, struct request_send * send, int n ){
	struct write_buffer * buf = append_sendbuffer_(ss, &s->low, send, SIZEOF_TCPBUFFER, n); 
	s->wb_size += buf->size; 
}

static int 
send_socket(struct socket_server *ss, struct request_send * send, struct socket_message *result, int priority){
	int id = send->id; 
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	if (s->type == SOCKET_TYPE_INVALID || s->type == SOCKET_TYPE_RESERVE ||
		s->id != id || s->type == SOCKET_TYPE_HALFCLOSE || s->type == SOCKET_TYPE_PACCEPT ||
		s->type == SOCKET_TYPE_CLOSE){
		return -1; 
	}

	if (s->type == SOCKET_TYPE_PLISTEN || s->type == SOCKET_TYPE_LISTEN){
		fprintf(stderr, "socket-server write to listen fd : %d\n", id); 
		return -1; 
	}

	if (send_buffer_empty(s) && s->type == SOCKET_TYPE_CONNECTED){
		if (s->protocol == IPPROTO_TCP){
			int n = write(s->fd, send->buffer, send->sz); 
			if (n < 0){
				switch(errno){
					case EINTR:
					case EAGAIN:
						n = 0;
						break; 
					default : 
						force_close(ss, s, result); 
						return SOCKET_CLOSE; 
				}
			}
			if (n == send->sz){
				return -1; 
			}

			append_sendbuffer(ss, s, send, n); 
		}else{
			//udp

		}
		sp_write(ss->epfd, s->fd, s, true); 
	}else{
		if (s->protocol == IPPROTO_TCP){
			if (priority == PRIORITY_HIGH){
				append_sendbuffer(ss, s, send, 0); 
			}else{
				append_sendbuffer_low(ss, s, send, 0); 
			}
		}else{
			//udp

		}
	}

	return -1; 
}

static int 
send_list_tcp(struct socket_server * ss, struct socket *s, struct wb_list *list, struct socket_message * result){
	while(list->head){
		struct write_buffer * tmp = list->head; 
		for(;;){
			int n = write(s->fd, tmp->ptr, tmp->size); 
			if(n < 0){
				switch (errno){
					case EINTR:
						continue; 
					case EAGAIN:
						return -1; 
					default: 
						force_close(ss, s, result); 
						return SOCKET_CLOSE; 
				}
			}

			s->wb_size -= n; 
			if (tmp->size != n){
				tmp->ptr += n; 
				tmp->size -= n; 
				return -1; 
			}
			break; 
		}	

		list->head = tmp->next; 
		write_buffer_free( tmp); 
	}
	list->tail = NULL; 
	return -1; 
}

static int 
send_list(struct socket_server *ss, struct socket *s, struct wb_list *wb, struct socket_message *result){
	if(s->protocol == IPPROTO_TCP){
		return send_list_tcp(ss, s, wb, result); 
	}
}

static int
list_uncomplete(struct wb_list *list){
	struct write_buffer *wb = list->head; 
	if(wb == NULL){
		return 0; 
	}

	return (void*)wb->ptr != wb->buffer; 
}

static void
raise_uncomplete(struct socket *s){
	struct wb_list *low = &s->low;
	struct write_buffer *tmp = low->head; 
	low->head = tmp->next; 
	if(low->head == NULL){
		low->tail = NULL; 
	}

	struct wb_list *high = &s->high; 
	assert(high->head == NULL); 

	tmp->next = NULL;
	high->head = high->tail = tmp; 
}

static int 
send_buffer(struct socket_server *ss, struct socket *s, struct socket_message *result){
	assert(!list_uncomplete(&s->low)); 

	if (send_list(ss, s, &s->high, result) == SOCKET_CLOSE){
		return SOCKET_CLOSE; 
	}

	if (s->high.head == NULL){
		if(s->low.head != NULL){
			if(send_list(ss, s, &s->low, result) == SOCKET_CLOSE){
				return SOCKET_CLOSE; 
			}

			if(list_uncomplete(&s->low)){
				raise_uncomplete(s); 
			}
		}else{
			sp_write(ss->epfd, s->fd, s, false); 
			if (s->type == SOCKET_TYPE_HALFCLOSE){
				force_close(ss, s,  result); 
				return SOCKET_CLOSE; 
			}
		}
	}
	return -1; 
}

static int 
close_socket(struct socket_server *ss, struct request_close *close, struct socket_message *result){
	int id = close->id; 
	struct socket *s = &ss->slot[HASH_ID(id)]; 
	if (s->type == SOCKET_TYPE_INVALID || id != s->id){
		result->id = id; 
		result->opaque = close->opaque; 
		result->ud = 0; 
		result->data = NULL; 
		return SOCKET_CLOSE; 
	}

	if (!send_buffer_empty(s)){
		int type = send_buffer(ss, s, result); 
		if (type != -1){
			return type; 
		}
	}

	if (send_buffer_empty(s) || close->shutdown ){
		force_close(ss, s, result); 
		result->id = id; 
		result->opaque = close->opaque; 
		return SOCKET_CLOSE; 
	}

	s->type = SOCKET_TYPE_HALFCLOSE; 

	return -1; 
}

static void 
socket_keepalive(int sock){
	int keepalive = 1; 
	setsockopt(sock, SOL_SOCKET, SOL_SOCKET, (void*)&keepalive, sizeof(int)); 
}

int 
open_socket(struct socket_server *ss, struct request_open *open, struct socket_message *result){
	int id = open->id; 
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	result->id = id; 
	result->opaque = open->opaque; 
	result->ud = 0; 
	result->data = NULL; 

	int status; 
	int new_sock; 
	struct socket *ns; 
	struct addrinfo ai_hints; 
	struct addrinfo *ai_list; 
	memset(&ai_hints, 0, sizeof(ai_hints)); 

	ai_hints.ai_family = AF_INET; 
	ai_hints.ai_socktype = SOCK_STREAM; 
	char service[16]; 
	sprintf(service, "%d", open->port); 

	status = getaddrinfo(open->host, service, &ai_hints, &ai_list); 
	if(status != 0 ){
		result->data = (void*)gai_strerror(status); 
		goto _failed; 
	}

	new_sock = socket(ai_list->ai_family, ai_list->ai_socktype, 0); 
	if(new_sock < 0){
		goto _failed; 
	}

	socket_keepalive(new_sock); 
	sp_nonblocking(new_sock); 

	status = connect(new_sock, ai_list->ai_addr, ai_list->ai_addrlen); 
	if(status != 0){
		result->data = strerror(errno); 
		goto _failed_fd; 
	}

	ns = new_fd(ss, id, new_sock, open->opaque, IPPROTO_TCP, true); 
	if(ns == NULL){
		result->data = "open sock failed"; 
		goto _failed_fd; 
	}

	if (status == 0){
		ns->type = SOCKET_TYPE_CONNECTED; 
		struct sockaddr *addr = ai_list->ai_addr; 
		void * sin_addr = (ai_list->ai_family == AF_INET) ? (void*)&((struct sockaddr_in*)addr)->sin_addr : (void*)&((struct sockaddr_in6*)addr)->sin6_addr; 
		if(inet_ntop(ai_list->ai_family, sin_addr, ss->buffer, sizeof(ss->buffer))){
			result->data = ss->buffer;
		}
		freeaddrinfo(ai_list); 
		return SOCKET_OPEN; 
	}else{
		ns->type = SOCKET_TYPE_CONNECTING; 
		sp_write(ss->epfd, new_sock, s, true); 
		return -1; 
	}

	freeaddrinfo(ai_list); 
	return -1; 

_failed_fd: 
	close(new_sock); 
_failed: 
	freeaddrinfo(ai_list); 
	return -1; 
}

static void
set_sockopt(struct socket_server *ss, struct request_setopt* setopt){
	int id = setopt->id; 
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	if(s->type == SOCKET_TYPE_INVALID || s->id != id){
		return ; 
	}
	int v = setopt->value; 
	setsockopt(s->fd, IPPROTO_TCP, setopt->what, (void*)&v, sizeof(int));
}

static int 
ctrl_cmd(struct socket_server *ss, struct socket_message * result){
	char buffer[256]; 
	char header[2];
	int sock = ss->recv_fd;  
	block_readpipe(sock, header, 2); 
	int type = header[0]; 
	int len = header[1]; 
	skynet_error("type: %d, len : %d", type, len); 
	block_readpipe(sock, buffer, len); 
	skynet_error("ctrl_cmd : %d", type); 
	switch(type){
		case 'L': 
			return listen_socket(ss, (struct request_listen*)buffer, result); 
		case 'S': 
			return start_socket(ss, (struct request_start*)buffer, result); 
		case 'D':
			return send_socket(ss, (struct request_send *)buffer, result, PRIORITY_HIGH); 
		case 'P': 
			return send_socket(ss, (struct request_send*)buffer, result, PRIORITY_LOW); 
		case 'K': 
			return close_socket(ss, (struct request_close*)buffer, result);
		case 'O':
			return open_socket(ss, (struct request_open*)buffer, result); 
		case 'T': 
			set_sockopt(ss, (struct request_setopt*)buffer); 
			return -1; 
		default: 
			fprintf(stderr, "socket-server , ctrl_cmd : %c\n", type); 
			return -1; 
	};

	return -1; 
}

static int 
report_accept(struct socket_server *ss, struct socket *s, struct socket_message *result){
	union sockaddr_all u; 
	int len = sizeof(u); 
	int fd = s->fd; 
	int client_fd = accept(fd, &u.s, &len); 
	if (client_fd < 0){
		if (errno == EMFILE || errno == ENFILE){
			result->opaque = s->opaque; 
			result->id = s->id; 
			result->ud = 0; 
			result->data = strerror(errno); 
			return -1; 
		}else{
			return 0; 
		}
	}

	int id = reserve_id(ss); 
	if(id < 0){
		close(client_fd); 
		return 0; 
	}

	socket_keepalive(client_fd); 
	sp_nonblocking(client_fd); 

	struct socket * ns = new_fd(ss, id, client_fd, s->opaque, IPPROTO_TCP, true); 
	if (ns == NULL){
		close(client_fd); 
		return 0; 
	}

	ns->type = SOCKET_TYPE_PACCEPT; 
	result->opaque = s->opaque; 
	result->id = s->id; 
	result->ud = id; 
	result->data = NULL; 

	void *sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void*)&u.v6.sin6_addr; 
	int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port); 
	char tmp[32]; 
	if(inet_ntop(u.s.sa_family, sin_addr, tmp, sizeof(tmp))){
		snprintf(ss->buffer, sizeof(ss->buffer), "%s:%d", tmp, sin_port); 
		result->data = ss->buffer; 
	}

	return 1; 
}

static void
clear_close_event(struct socket_server *ss, struct socket_message *result, int type){
	if(type == SOCKET_ERROR || type == SOCKET_CLOSE){
		int id = result->id; 
		int i; 
		for(i=ss->event_index;i<ss->event_n;i++){
			struct event * e = &ss->evs[i]; 
			struct socket * s = (struct socket*)e->ptr; 
			if (s){
				if(s->type == SOCKET_TYPE_INVALID && s->id == id){
					e->ptr = NULL; 
					break; 
				}
			}
		}
	}
}

static int
forword_message_tcp(struct socket_server *ss, struct socket *s, struct socket_message *result){
	int sz = s->p.sz; 
	char * buf = malloc(sz); 
	int n = read(s->fd, buf, sz); 
	if(n < 0){
		free(buf); 
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN: 
				break; 
			default : 
				force_close(ss, s, result); 
				result->data = strerror(errno); 
				return SOCKET_ERROR; 
		}
	}

	if (n == 0){
		free(buf); 
		force_close(ss, s, result); 
		return SOCKET_CLOSE; 
	}

	if(s->type == SOCKET_TYPE_HALFCLOSE){
		free(buf);
		return -1; 
	}

	if (n == sz){
		s->p.sz *=2; 
	}else if(sz >= MIN_BUFFER_SIZE && n*2 < sz){
		s->p.sz /= 2; 
	}

	result->opaque = s->opaque; 
	result->id = s->id; 
	result->ud = n; 
	result->data = buf; 

	return SOCKET_DATA; 
}

int 
socket_server_poll(struct socket_server *ss, struct socket_message *result){
	for(;;){
		if(ss->check_ctrl){
			if(has_cmd(ss)){
				int type = ctrl_cmd(ss, result); 
				if (type != -1){
					clear_close_event(ss, result, type); 
					return type; 
				}else{
					continue; 
				}
			}else{
				ss->check_ctrl = 0;
			}
		}

		if(ss->event_index == ss->event_n){
			ss->event_n = sp_wait(ss->epfd, ss->evs, MAX_EVENTS); 
			skynet_error("event_n : %d", ss->event_n); 
			ss->check_ctrl = 1; 
			ss->event_index = 0; 
			if (ss->event_n <= 0){
				ss->event_n = 0; 
				return -1; 
			}
		}

		struct event *e = &ss->evs[ss->event_index++]; 
		struct socket * s= (struct socket*)e->ptr; 

		if (s == NULL){
			continue; 
		}

		switch(s->type){
			case SOCKET_TYPE_LISTEN: {
				int ok = report_accept(ss, s, result); 
				if (ok > 0){
					return SOCKET_ACCEPT; 
				}else{
					return SOCKET_ERROR; 
				}
				break;
			}
			default: {
				if (e->read){
					int type; 
					if(s->protocol == IPPROTO_TCP){
						type = forword_message_tcp(ss, s, result); 
					}else{
						//udp
					}

					if (e->write && (type != SOCKET_CLOSE && type != SOCKET_ERROR)){
						e->read = false; 
						--ss->event_index; 
					}

					if (type == -1)
						break; 
					return type;
				}

				if (e->write){
					int type = send_buffer(ss, s, result); 
					if (type == -1)
						break; 
					return type; 
				}
				break; 
			}//default
		}
	}
}


int 
do_bind(const char *host, int port, int protocol, int *family){
	int status; 
	int sock; 
	int reuse = 1; 
	struct addrinfo ai_hints ; 
	struct addrinfo *ai_list; 
	memset(&ai_hints, 0, sizeof(ai_hints)); 

	ai_hints.ai_family = AF_INET; 
	if (protocol == IPPROTO_TCP){
		ai_hints.ai_socktype = SOCK_STREAM; 
	}else{
		assert(protocol == IPPROTO_UDP); 
		ai_hints.ai_socktype = SOCK_DGRAM; 
	}
	ai_hints.ai_protocol = protocol ; 

	if (host == NULL || host[0] == 0){
		host = "0.0.0.0"; 
	}
	char service[16]; 
	sprintf(service, "%d", port);
	status = getaddrinfo(host, service, &ai_hints, &ai_list); 
	if(status == -1){
		return -1; 
	}

	*family = ai_list->ai_family; 
	sock = socket(ai_list->ai_family, ai_list->ai_socktype, 0); 
	if(sock < 0){
		goto _failed_fd; 
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1){
		goto _failed; 
	}

	status = bind(sock, ai_list->ai_addr, ai_list->ai_addrlen); 
	if(status == -1){
		goto _failed_fd; 
	}

	freeaddrinfo(ai_list); 
	return sock; 
_failed: 
	close(sock); 
_failed_fd: 
	freeaddrinfo(ai_list); 
	return -1; 
}

static int 
do_listen(const char *host, int port, int protocol, int backlog){
	int family = 0; 
	int sock = do_bind(host, port, protocol, &family); 
	if (sock == -1){
		return -1; 
	}

	int ret = listen(sock, backlog);
	if (ret == -1){
		close(sock); 
		return -1; 
	}
	return sock; 
}

int 
socket_server_listen(struct socket_server*ss, int opaque, const char *host, int port, int backlog){
	int listen_sock = do_listen(host, port, IPPROTO_TCP, backlog); 
	if(listen_sock == -1){
		return -1; 
	}

	int id = reserve_id(ss); 
	if (id == -1){
		return -1; 
	}

	skynet_error("listen_sock: %d, id :  %d, sz: %d", listen_sock, id, sizeof(struct request_listen)); 

	struct request_package package; 
	package.u.listen.id = id; 
	package.u.listen.fd = listen_sock; 
	package.u.listen.opaque = opaque; 

	send_request(ss, &package, 'L', sizeof(package.u.listen)); 

	return id; 
}

int 
socket_server_start(struct socket_server *ss, int opaque, int id){
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	assert(s->id == id); 

	struct request_package package; 
	package.u.start.id = id; 
	package.u.start.opaque = opaque; 

	send_request(ss, &package, 'S', sizeof(package.u.start)); 
	return id; 
}

int 
socket_server_send(struct socket_server *ss, int id, char * buffer, int sz){
	struct socket * s= &ss->slot[HASH_ID(id)]; 
	if(s->type == SOCKET_TYPE_INVALID || s->id != id){
		return -1; 
	}

	struct request_package package; 
	package.u.send.id = id; 
	package.u.send.sz = sz; 
	package.u.send.buffer = buffer; 

	send_request(ss, &package, 'D', sizeof(package.u.send)); 

	return s->wb_size; 
}

void
socket_server_send_lowpriority(struct socket_server *ss, int id, char *buffer, int sz){
	struct socket * s = &ss->slot[HASH_ID(id)]; 
	if (s->type == SOCKET_TYPE_INVALID || s->id != id){
		return; 
	}

	struct request_package package; 
	package.u.send.id = id; 
	package.u.send.sz = sz; 
	package.u.send.buffer = buffer; 

	send_request(ss, &package, 'P', sizeof(package.u.send)); 
}

void
socket_server_close(struct socket_server *ss, int opaque, int id){
	struct request_package package; 
	package.u.close.opaque = opaque; 
	package.u.close.id = id; 
	package.u.close.shutdown = 0; 

	send_request(ss, &package, 'K', sizeof(package.u.close)); 
}

void
socket_server_shutdown(struct socket_server *ss, int opaque, int id){
	struct request_package package; 
	package.u.close.opaque = opaque; 
	package.u.close.id = id; 
	package.u.close.shutdown = 1; 

	send_request(ss, &package, 'K', sizeof(package.u.close)); 
}

static int 
open_request(struct socket_server *ss, struct request_package *request, int opaque, const char * host, int port){
	int len = strlen(host); 
	if(len + sizeof(request->u.open) >= 256){
		return -1; 
	}

	int id = reserve_id(ss); 
	if (id < 0){
		return -1; 
	}

	request->u.open.opaque = opaque; 
	request->u.open.id = id;
	request->u.open.port = port; 
	memcpy(request->u.open.host, host, len); 
	request->u.open.host[len] = '\0'; 

	return len; 
}

int 
socket_server_connect(struct socket_server *ss, int opaque, const char *host, int port){
	struct request_package package; 
	int len  = open_request(ss, &package, opaque,  host, port); 
	if (len < 0){
		return -1; 
	}
	send_request(ss, &package, 'O',  sizeof(package.u.open)+len); 
	return package.u.open.id; 
}

void
socket_server_nodelay(struct socket_server *ss, int id){
	struct request_package package; 
	package.u.setopt.id = id; 
	package.u.setopt.what = TCP_NODELAY; 
	package.u.setopt.value = 1; 

	send_request(ss, &package, 'T', sizeof(package.u.setopt)); 
}