#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/epoll.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>


#define PORT 8888
#define HOST "0.0.0.0"
#define LOG_MSG_SIZE 256


struct socket_server{
	int epfd; 
	int listen_sock; 
};

struct socket_server * S = NULL; 

void
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	memset(msg, 0, LOG_MSG_SIZE); 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}

int 
main(int argc, char *argv[]){

	setInfo("EPOLLIN: %d, EPOLLOUT: %d, EPOLLERR: %d, EPOLLHUP: %d, EPOLLRDHUP: %d, EPOLLPRI: %d", \
		EPOLLIN, EPOLLOUT, EPOLLERR, EPOLLHUP, EPOLLRDHUP, EPOLLPRI); 

	S = malloc(sizeof(*S)); 
	memset(S, 0, sizeof(*S)); 
	S->epfd = 0; 
	S->listen_sock = 0; 

	S->epfd = epoll_create(1024); 

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, HOST, &addr.sin_addr); 

	int sock; 
	int ret; 
	sock = socket(AF_INET, SOCK_STREAM, 0); 

	ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr)); 

	ret = listen(sock, 5);

	S->listen_sock = sock;  

	struct epoll_event ev; 
	ev.events = EPOLLIN; 
	ev.data.fd = S->listen_sock; 

	ret = epoll_ctl(S->epfd, EPOLL_CTL_ADD, S->listen_sock, &ev); 

	for(;;){
		struct epoll_event evs[50];
		int i, n; 
		n = epoll_wait(S->epfd, evs, 50, -1); 
		setInfo("n: %d", n); 
		for(i=0;i<n;i++){
			int sock = evs[i].data.fd; 
			int events = evs[i].events; 

			setInfo("sock : %d, events : %d", sock, events); 

			if (sock == S->listen_sock){
				struct sockaddr_in new_addr; 
				int new_len = sizeof(new_addr); 
				int new_sock = accept(S->listen_sock, (struct sockaddr*)&new_addr, &new_len); 

				struct epoll_event ev; 
				ev.events = EPOLLIN; 
				ev.data.fd = new_sock; 
				epoll_ctl(S->epfd, EPOLL_CTL_ADD, new_sock, &ev); 

			}else if (EPOLLIN & events){
				setInfo("kkk"); 
				int len; 
				char msg[LOG_MSG_SIZE]; 
				memset(msg, 0, LOG_MSG_SIZE); 
				len = read(sock, msg, LOG_MSG_SIZE); 
				setInfo("%d  %s", len, msg); 

				if (len < 0){
					switch(errno){
						case EINTR: 
							fprintf(stderr, "EINTR : %s\n", strerror(errno));
							break; 
						case EAGAIN : 
							fprintf(stderr, "EAGAIN : %s\n", strerror(errno)); 
							break; 
						default: 
							fprintf(stderr, "default : %s\n", strerror(errno)); 
							epoll_ctl(S->epfd, EPOLL_CTL_DEL, sock, NULL); 
							close(sock); 
							continue; 
					}
				}

				if (len == 0){
					epoll_ctl(S->epfd, EPOLL_CTL_DEL, sock, NULL); 
					close(sock); 
					continue; 
				}

				char write_msg[LOG_MSG_SIZE] = "msg - from - server"; 
				int write_len; 
				write_len = write(sock, write_msg, strlen(write_msg)); 
			}else if (EPOLLOUT && events){
				char write_msg[LOG_MSG_SIZE] = "msg - from - server"; 
				int write_len; 
				write_len = write(sock, write_msg, strlen(write_msg)); 
			}else{
				setInfo("error !!!!");
				continue; 
			}
		}
	} // for(;;)

	epoll_ctl(S->epfd, EPOLL_CTL_DEL, S->listen_sock, NULL); 
	close(S->listen_sock); 
	close(S->epfd); 
	free(S); 

	setInfo("over !!!!"); 

	return 0; 
}