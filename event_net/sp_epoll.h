#ifndef epoll_h
#define epoll_h

#include <sys/epoll.h>

#include "sp_error.h"
#include "event.h"
#include "socket_server.h"

#define MAX_EVENTS 50

static int 
sp_create(){
	return epoll_create(1024); 
}

static int 
sp_invalid(int sock){
	return (sock == 0) ? 1 : 0; 
}

static int 
sp_add(int epfd, int sock, void *ud){
	struct epoll_event ev; 
	ev.events = EPOLLIN; 
	ev.data.ptr = ud; 

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1){
		return -1; 
	}
	return 0; 
}

static int 
sp_modify(int epfd, int sock, void *ud){
	struct epoll_event ev; 
	ev.events = EPOLLIN | EPOLLOUT; 
	ev.data.ptr = ud; 

	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev) == -1){
		return -1; 
	}
	return 0; 
}

static void
sp_del(int epfd, int sock){
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL); 
}

static int
sp_wait(int epfd, int max){
	int n; 
	int i; 
	struct epoll_event events[max]; 
	n = epoll_wait(epfd, events, max, -1); 
	for(i=0;i<n;i++){
		struct event * ev = (struct event*)events[i].data.ptr; 
		int evs = events[i].events; 

		set_info("sock : %d, evs : %d", ev->sock, evs);

		if (evs & EPOLLIN){
			int m = ev->recv_handler(epfd, ev->sock); 
			if (m > 0){
				ev->write_handler(epfd,  ev->sock); 
			}
		}else if(evs & EPOLLOUT){
			ev->write_handler(epfd, ev->sock); 
		}else{
			set_error("%s", "error events"); 
			break; 
		}
	}

	return n; 
}

static void
epoll_dispatch(int epfd){
	for(;;){
		sp_wait( epfd, MAX_EVENTS ); 
	}
}

#endif