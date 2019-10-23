#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>


#define PORT 8888
#define HOST "127.0.0.1"
#define LOG_MSG_SIZE 256

void setInfo(const char *fmt, ...); 


int
main(int argc, char * argv[]){

	int sock; 
	int ret; 

	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock < 0){
		fprintf(stderr, "socket failed, sock : %d", sock); 
		return -1; 
	}

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if (ret < 0){
		fprintf(stderr, "connect socket failed, new_sock : %d", ret); 
		return -1; 
	}

	printf("sock : %d", sock); 

	int n; 
	char msg[LOG_MSG_SIZE] = "msg from client"; 
	n = write(sock, msg, LOG_MSG_SIZE); 

	printf("n = %d", n); 

	return 0;
}

void 
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 
	perror(msg); 
}