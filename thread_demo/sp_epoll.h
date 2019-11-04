#ifndef sp_epoll_h
#define sp_epoll_h

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>


static int 
sp_create(){
	return epoll_create(1024); 
}

static int 
sp_invalid(int sock){
	return sock == -1; 
}

static void 
sp_add(int epfd, int sock,  void *ud){
	struct epoll_event ev; 
	ev.events = EPOLLIN; 
	ev.data.ptr = ud; 

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev)){
		return -1; 
	}
	return 0; 
}

static void
sp_del(int epfd, int sock){
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL); 
}

static int 
sp_wait(int epfd, struct epoll_event *evs, int max){
	int n; 
	n = epoll_ctl(epfd, evs, max, -1); 
	return n;  
}

static void 
sp_nonblocking(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flags); 
}


#endif