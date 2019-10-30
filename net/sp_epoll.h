#ifndef sp_epoll_h
#define sp_epoll_h

#include <sys/epoll.h>
#include <fcntl.h>

#include "sp_error.h"

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

static int
sp_write(int epfd, int sock, void *ud){
	struct epoll_event ev; 
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLPRI; 
	ev.data.ptr = ud; 
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev) != 0){
		return -1; 
	}
	return 0; 
}

static int 
sp_wait(int epfd, struct event *uevs){
	int n; 
	int i; 
	struct epoll_event evs[50]; 
	n = epoll_wait(epfd, evs, 50, -1); 
	for (i=0;i<n;i++){
		struct event* ev = (struct event*)evs[i].data.ptr; 
		uevs[i].sock = ev->sock; 
		uevs[i].read = (evs[i].events & EPOLLIN) ? 1: 0; 
		uevs[i].write = (evs[i].events & EPOLLOUT) ? 1 : 0;  
		uevs[i].ptr = ev->ptr; 
		setInfo("sp_wait, sock: %d, read: %d, write: %d", uevs[i].sock, uevs[i].read, uevs[i].write );
	}
	return n; 
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