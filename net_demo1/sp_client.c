#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>

#define PORT 8880
#define LOG_MAX_SIZE 256
#define HOST "127.0.0.1"


int do_socket();
int do_connect(int sock); 
void sp_nonblocking(int sock); 
void sp_reuse(int sock); 
void write_info(int sock); 
void recv_info(int sock); 
void setInfo(const char *fmt, ...);
void setError(const char *fmt, ...); 

int
main(int argc, char *argv[]){

	int sock;
	int ret ; 

	sock = do_socket(); 
	if (sock == -1){
		setError("do-socket,failed, %d", sock); 
		return -1; 
	}

	ret = do_connect(sock); 
	if (ret == -1){
		setError("do-connect, failed, %d", ret); 
		return -1;
	}

	write_info(sock); 

	sleep(1);

	recv_info(sock); 

	return 0; 
}

void 
setError(const char *fmt , ...){
	va_list ap ; 
	va_start(ap , fmt); 
	char msg[LOG_MAX_SIZE]; 
	vsnprintf(msg, LOG_MAX_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}

void
setInfo(const char * fmt, ...){
	va_list ap ; 
	va_start(ap, fmt); 
	char msg[LOG_MAX_SIZE]; 
	vsnprintf(msg, LOG_MAX_SIZE, fmt, ap); 
	printf("%s", msg); 
	va_end(ap); 
}

void 
write_info(int sock){
	int n; 
	char msg[LOG_MAX_SIZE] = "MSG FROM CLIENT";
	write(sock, msg, LOG_MAX_SIZE); 
}

void 
recv_info(int sock){
	int n; 
	char msg[LOG_MAX_SIZE]; 
	n = read(sock, msg, LOG_MAX_SIZE); 
	setInfo("msg: %s", msg); 
}


int 
do_connect(int sock){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	int ret; 
	ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if (ret < 0){
		fprintf(stderr, "connect sock failed, ret : %d", ret); 
		return -1; 
	}

	// sp_nonblocking(new_sock); 
	// sp_reuse(new_sock); 

	return ret; 
}

int 
do_socket(){
	int sock = socket(AF_INET, SOCK_STREAM, 0); 
	if(sock < 0){
		fprintf(stderr, "socket failed, sock: %d", sock); 
		return -1; 
	}
	// sp_nonblocking(sock); 
	return sock; 
}

void 
sp_reuse(int sock){
	int reuse = 1; 
	int ret; 
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)); 
	if (ret != 0){
		fprintf(stderr, "setsockopt reuse failed, sock: %d, ret: %d", sock, ret); 
		return ; 
	}
}

void
sp_nonblocking(int sock){
	int flag = fcntl(sock, F_GETFL, 0); 
	flag |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flag); 
}