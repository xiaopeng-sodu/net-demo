#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#define HOST "0.0.0.0"
#define PORT 8888
#define LOG_MSG_SIZE 256

int max = 1; 

void setError(const char *fmt, ...); 
void setInfo(const char * fmt, ...); 
int do_socket(); 
void set_nonblock(int sock); 
void set_reuseaddr(int sock); 
int write_info(int sock); 
int recv_info(int sock); 

int 
main(int argc, char *argv[]){
	int sock; 
	int ret; 

	sock = do_socket(); 
	if (sock == -1){
		setError("do_socket failed, sock: %d\n", sock); 
		return -1;
	}

	setInfo("sock : %d", sock); 

	write_info(sock); 

	sleep(1); 


	// recv_info(sock);

	// close(sock); 

	sleep(10000);

	return 0; 
}

int 
write_info(int sock){
	char msg[LOG_MSG_SIZE] = "msg from client"; 
	int n = 0; 
	// int len = strlen(msg); 
	// for(;;){
		n = write(sock, msg, LOG_MSG_SIZE); 
		// if (n != len){
			// msg = msg + n; 
		// }else{
			// break; 
		// }	
	// }
	return n; 
}

int 
recv_info(int sock){
	char msg[LOG_MSG_SIZE]; 
	int n; 
	n = read(sock, msg, LOG_MSG_SIZE); 
	setInfo("recv_info n : %d, msg : %s", n, msg); 
	return n; 
}

void 
set_reuseaddr(int sock){
	int reuse = 1; 
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(reuse)); 
}

int
do_socket(){
	int ret; 
	int sock; 
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	ret = inet_pton(AF_INET, HOST, &addr.sin_addr); 
	if (ret != 1){
		fprintf(stderr, "inet_pton failed ret : %d, error: %s\n", ret, strerror(errno)); 
		return -1; 
	}

	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if(sock <= 0){
		fprintf(stderr, "socket failed, sock: %d, error: %s\n", sock, strerror(errno)); 
		return -1; 
	}

	ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if (ret != 0){
		fprintf(stderr, "connect failed, ret: %d, error: %s\n", ret, strerror(errno)); 
		return -1; 
	}

	set_nonblock(sock); 
	set_reuseaddr(sock); 

	return sock; 
}

void 
set_nonblock(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flags); 
}

void
setError(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}

void 
setInfo(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg);
	va_end(ap); 
}