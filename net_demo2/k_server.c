#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8888
#define HOST "127.0.0.1"
#define LOG_MSG_SIZE 256 
#define MAX_EVENTS 50

int epfd; 
int listenfd; 
static int max = 0; 

void setInfo(const char * fmt, ...); 

int 
main(int argc, char * argv[]){
	int ret; 
	int n; 
	epfd = epoll_create(1024); 
	setInfo("epfd : %d", epfd); 

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setInfo("listenfd : %d", listenfd); 

	ret = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)); 

	ret = listen(listenfd, 5); 

	struct epoll_event event; 
	event.events = EPOLLIN; 
	event.data.fd = listenfd; 
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event); 

	struct epoll_event evs[MAX_EVENTS]; 

	for(;;){
		n = epoll_wait(epfd, evs, MAX_EVENTS, -1); 
		setInfo("n - epoll_wait : %d", n); 
		int i; 
		max += 1; 
		for(i=0;i<n;i++){
			int sock = evs[i].data.fd; 
			int events = evs[i].events; 

			setInfo("epoll-wait-sock , sock: %d, events : %d", sock, events); 

			if(sock == listenfd){

				struct sockaddr_in new_addr; 
				int new_addr_len = sizeof(new_addr); 
				int new_sock = accept(listenfd, (struct sockaddr*)&new_addr, &new_addr_len); 
				setInfo("accept - new_sock : %d", new_sock); 

				struct epoll_event event; 
				event.events = EPOLLIN; 
				event.data.fd = new_sock; 

				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, new_sock, &event); 

			}else if (events & EPOLLIN){

				char msg[LOG_MSG_SIZE]; 
				memset(msg, 0, LOG_MSG_SIZE); 
				int n = read(sock, msg, LOG_MSG_SIZE); 
				setInfo("read - n: %d, msg:  %s", n, msg); 

				if (n == 0){
					epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL); 
					close(sock); 
					continue; 
				}

				memset(msg, 0, LOG_MSG_SIZE); 
				char cmsg[LOG_MSG_SIZE] = "from serverf; "; 
				n = write(sock, cmsg, LOG_MSG_SIZE); 

			}else if (events & EPOLLOUT){

				char msg[] = "msg-from-server\n"; 
				int n = write(sock, msg, strlen(msg)); 
				setInfo("write - n : %d, msg : %s", n ,msg); 

			}else{

				setInfo("epoll-wait failed"); 
				return 0; 
			}
		}

		if (max >= 3){
			// return 0; 
		}
		// n = 0; 
	}

	return 0; 
}

void 
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap);
}

