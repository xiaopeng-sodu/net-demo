#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#define PORT 8889
#define HOST "127.0.0.1"
#define LOG_MSG_SIZE 256

int do_socket(); 
int do_connect(int sock); 
void setError(const char * fmt, ...); 
void sp_nonblocking(int sock); 
int write_info(int sock);
int recv_info(int sock); 
void setInfo(const char * fmt, ...);

int
main(int argc, char * argv[]){

	int sock;
	int ret; 

	sock = do_socket(); 

	if (sock == -1){
		setError("do_socket failed , sock = %d", sock); 
		return -1; 
	}

	setInfo("sock : %d\n", sock); 

	ret = do_connect(sock); 

	if(ret == -1){
		setError("do_connect failed, ret = %d", ret); 
		return -1; 
	}

	write_info(sock); 

	sleep(10); 

	recv_info(sock); 

	sleep(1000); 

	return 1; 
}

void 
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}

int 
recv_info(int sock){
	char msg[LOG_MSG_SIZE]; 
	int n = read(sock, msg, LOG_MSG_SIZE); 
	setInfo("n: %d, msg: %s", n, msg); 
	return n; 
}

int 
write_info(int sock){
	char msg[LOG_MSG_SIZE] = "msg from client"; 
	int n = write(sock, msg, LOG_MSG_SIZE); 
	return n; 
}

int 
do_socket(){
	int sock ;

	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock <= 0){
		fprintf(stderr, "socket failed, sock : %d, error: %s\n", sock, strerror(errno));
		return -1; 
	}

	// sp_nonblocking(sock); 

	return sock; 
}

int 
do_connect(int sock){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT);
	inet_aton(HOST, &addr.sin_addr); 

	int ret; 
	ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)); 
	if (ret != 0){
		fprintf(stderr, "connect sock, failed, ret: %d, error: %s\n", ret, strerror(errno)); 
		return -1; 
	}

	return ret; 
}

void 
sp_nonblocking(int sock){
	int flag = fcntl(sock, F_GETFL, 0); 
	flag |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flag); 
}

void
setError(const char * fmt, ...){
	va_list ap ; 
	va_start(ap, fmt); 
	char msg[LOG_MSG_SIZE]; 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}