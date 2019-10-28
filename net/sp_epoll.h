#ifndef sp_epoll_h
#define sp_epoll_h

#include <sys/epoll.h>
#include <fcntl.h>

struct event {
	int sock; 
	int read; 
	int write; 
	void * ptr; 
};

static int 
sp_create(){
	return epoll_create(1024); 
}

static int 
sp_invalid(int sock){
	return sock <= 0; 
}

static void
sp_del(int epfd, int sock){
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL);
}

static int 
sp_add(int epfd, int sock, void *ud){
	struct epoll_event ev;
	ev.events = EPOLLIN; 
	ev.data.ptr = ud; 
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) != 0){
		return -1; 
	}
	return 0;
}

static void 
set_nonblocking(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flags); 
}

static void 
set_reuse(int sock){
	int reuse = 1; 
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1){
		fprintf(stderr, "setsockopt SO_REUSE failed"); 
		return;
	}
}


#endif