#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define LOG_MSG_SIZE 256
#define PORT 8880
const char * HOST = "127.0.0.1";

int listenfd; 
int clientfd; 

void setError(const char *fmt, ...);
void setInfo(const char* fmt, ...);
int do_listen();
void sp_nonblocking(int sock);
int do_accept(); 
int read_info(int sock);
int write_info(int sock); 

int 
main(int argc, char *argv[]){

	listenfd = do_listen();
	if(listenfd == -1){
		setError("do_listen failed, listenfd: %d, strerror: %s", listenfd, strerror(errno)); 
		return -1; 
	}

	setInfo("listenfd: %d", listenfd);

	/*clientfd = do_accept(); 
	if(clientfd == -1){
		setError("do accept failed, clientfd: %d, strerror: %s", clientfd, strerror(errno)); 
		return -1; 
	}*/

	int len; 
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(struct sockaddr_in)); 
	len = sizeof(struct sockaddr_in); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	clientfd = accept(listenfd, (struct sockaddr*)&addr, &len); 
	setInfo("new_sock: %d", clientfd); 

	if (clientfd < 0){
		fprintf(stderr, "accept socket failed, new_sock: %d, strerror: %s", clientfd, strerror(errno)); 
		return -1; 
	}

	read_info(clientfd); 
	// int n; 
	// char msg[LOG_MSG_SIZE]; 
	// n = read(clientfd, msg, LOG_MSG_SIZE); 
	// setInfo("n : %d, msg: %s", n, msg);

	write_info(clientfd);

	return 0;
}

int
write_info(int sock){
	int n; 
	char msg[LOG_MSG_SIZE] = "MSG FROM SERVER"; 
	n = write(sock, msg, LOG_MSG_SIZE); 
	return n; 
}

void 
setInfo(const char* fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	//printf("%s", msg); 
	perror(msg);
	va_end(ap); 
}

int 
read_info(int sock){
	int n ; 
	char msg[LOG_MSG_SIZE]; 
	n = read(sock, msg, LOG_MSG_SIZE); 
	if (n < 0){
		return -1;
	}
	setInfo("msg:%s", msg); 
	return n; 
}

int 
do_accept(){
	int len; 
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(struct sockaddr_in)); 
	len = sizeof(struct sockaddr_in); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	int new_sock = accept(listenfd, (struct sockaddr*)&addr, &len); 
	if (new_sock < 0){
		fprintf(stderr, "accept socket failed, new_sock: %d, strerror: %s", new_sock, strerror(errno)); 
		return -1; 
	}

	return new_sock; 
}

int 
do_listen(){
	int ret ; 
	int sock; 
	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if(sock < 0){
		fprintf(stderr, "create-socket failed , sock : %d, strerror: %s", sock, strerror(errno)); 
		return -1;
	}

	//sp_nonblocking(sock); 
	/*int reuse =1; 
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)); 
	if (ret != 0){
		fprintf(stderr, "setsockopt reuse failed, ret : %d, strerror : %s", ret, strerror(errno)); 
		return -1; 
	}
	*/


	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT);
	inet_aton(HOST, &addr.sin_addr);

	ret = bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if(ret != 0){
		fprintf(stderr, "bind socket failed , ret : %d, strerror : %s", ret, strerror(errno));
		return -1;
	}

	ret = listen(sock, 5); 
	if(ret != 0){
		fprintf(stderr, "listen socket failed, ret: %d, strerror : %s", ret, strerror(errno)); 
		return -1;
	}

	return sock; 
}

void
sp_nonblocking(int sock){
	int flag = fcntl(sock, F_GETFL, 0); 
	flag |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flag); 
}


void 
setError(const char *fmt, ...){
	char msg[LOG_MSG_SIZE];
	va_list ap; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap);
	va_end(ap); 
	perror(msg);
}