#ifndef socket_epoll_h
#define socket_epoll_h

#include <sys/epoll.h>
#include <fcntl.h>
#include <stdbool.h>

struct event {
	void * ptr; 
	int read; 
	int write; 
};


static int 
sp_create(){
	return epoll_create(1024); 
}

static int 
sp_invalid(int sock){
	return sock < 0 ? -1 : 0; 
}

static int 
sp_add(int epfd, int sock, void *ud){
	struct epoll_event event; 
	event.events = EPOLLIN; 
	event.data.ptr = ud; 
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event) == -1){
		return -1;
	}
	return 0; 
}

static int 
sp_write(int epfd, int sock, void *ud, bool write){
	struct epoll_event event; 
	event.events = EPOLLIN | (write ? EPOLLOUT : 0); 
	event.data.ptr = ud; 
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &event) == -1){
		return -1;
	}
	return 0; 
}

static void
sp_del(int epfd, int sock){
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL); 
}

static int 
sp_wait(int epfd, struct event *evs, int max){
	struct epoll_event events[max]; 
	int i, n; 
	n = epoll_wait(epfd, events, max, -1); 
	for(i=0;i<n;i++){
		evs[i].read = (events[i].events & EPOLLIN) ? 1 : 0; 
		evs[i].write = (events[i].events & EPOLLOUT) ? 1 : 0; 
		evs[i].ptr = events[i].data.ptr; 
	} 

	return n; 
}

static int 
sp_nonblocking(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	return fcntl(sock, F_SETFL, flags); 
}

#endif