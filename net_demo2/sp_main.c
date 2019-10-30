#include <stdio.h>
#include <stdarg.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#define LOG_MAX_SIZE 256
#define PORT 8888
#define HOST "127.0.0.1"


int epfd;
int listenfd;


void setError(const char * fmt, ...);
void setInfo(const char *fmt, ...);
int do_listen();
void sp_noblocking(int sock);
int sp_add(int sock);
int sp_wait(struct epoll_event * evs); 
int do_accept();
void recv_info(int sock); 
void write_info(int sock);


int
main(int argc, char * argv[]){
	epfd = epoll_create(1024); 
	if (epfd <= 0){
		setError("create epoll-fd failed, epfd : %d", epfd);
		return -1;
	}

	// setError("epfd = %d", epfd); 

	listenfd = do_listen();
	if (listenfd == -1){
		setError("do_listen failed = %d ", listenfd);
		return -1; 
	}

	// setError("listenfd = %d", listenfd); 

	int ret ; 
	ret = sp_add(listenfd); 
	if (ret != 0){
		setError("sp_add failded , ret = %d", ret);
		return -1; 
	}

	struct epoll_event evs[50]; 
	for(;;){
		int i; 
		int n; 
		n = sp_wait(evs); 
		setInfo("n: %d", n); 
		for(i=0;i<n;i++){

			int sock ;
			int events ;

			sock = evs[i].data.fd;
			events = evs[i].events;

			setInfo("sock : %d, events: %d", sock, events); 

			if (sock == listenfd)
			{
				int new_sock ;
				new_sock= do_accept();
				setInfo("new_sock == %d", new_sock); 
				sp_noblocking(new_sock); 
				sp_add(new_sock);
			}
			else if(events & EPOLLIN)
			{
				recv_info(sock); 
				// write_info(sock); 
			}
			else if (events & EPOLLOUT)
			{
				write_info(sock); 
			}
			else{
				setError("error!");
				break; 
			}
		}
	}

	return 0; 
}

void 
write_info(int sock){
	char msg[LOG_MAX_SIZE] = "MSG FROM SERVER"; 
	int n = write(sock, msg, LOG_MAX_SIZE); 
	setInfo("n: %d, msg: %s",  n , msg);
}

void
recv_info(int sock){
	char msg[LOG_MAX_SIZE]; 
	int n = read(sock, msg, LOG_MAX_SIZE); 
	setInfo("len: %d, msg: %s", n, msg);
}

void 
setInfo(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt); 
	char msg[LOG_MAX_SIZE]; 
	vsnprintf(msg, LOG_MAX_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}

int 
do_accept(){
	struct sockaddr_in addr; 
	int len = sizeof(struct sockaddr_in);
	memset(&addr, 0, len); 
	int sock; 
	sock = accept(listenfd, (struct sockaddr*)&addr, &len); 
	if (sock <= 0){
		setError("accept failed , sock : %d", sock); 
		return -1; 
	}
	char msg[LOG_MAX_SIZE]; 
	const char * ip = inet_ntop(AF_INET, &addr.sin_addr, msg, LOG_MAX_SIZE); 
	setInfo("ip : %s, msg : %s", ip, msg); 
	return sock;
}


int 
sp_wait(struct epoll_event* evs){
	int n = epoll_wait(epfd, evs, 50, -1); 
	return n;
}

int 
sp_add(int sock){
	struct epoll_event ev; 
	ev.events = EPOLLIN; 
	ev.data.fd = sock;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1){
		return -1;
	}
	return 0;
}

int 
do_listen(){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT);
	// inet_aton(HOST, &addr.sin_addr);
	inet_pton(AF_INET, HOST, &addr.sin_addr); 

	int sock = socket(AF_INET, SOCK_STREAM, 0); 
	if(sock <= 0){
		setError("socket failed , sock = %d", sock);
		return -1;
	}

	int reuse = 1; 
	int ret; 
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int));
	if (ret == -1){
		setError("setsockopt reuse failed , sock = %d", sock);
		return -1;
	}

	sp_noblocking(sock);

	ret = bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if (ret != 0){
		setError("bind failed , sock = %d", sock);
		return -1; 
	}

	ret = listen(sock, 5);
	if (ret == -1){
		setError("listen failed , sock = %d", sock);
		return -1; 
	}

	return sock;
}

void 
sp_noblocking(int sock){
	int flag = fcntl(sock, F_GETFL, 0);
	if (flag == -1){
		return;
	}
	flag |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flag); 
}

void 
setError(const char * fmt, ...){
	va_list ap ; 
	va_start(ap, fmt);
	char msg[LOG_MAX_SIZE];
	vsnprintf(msg, LOG_MAX_SIZE, fmt, ap);
	perror(msg);
	va_end(ap);
}