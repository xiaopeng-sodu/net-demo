#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>



#define HOST "0.0.0.0"
#define PORT 8888
#define LOG_MSG_SIZE 256

void
setInfo(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}


int 
main(int argc, char *argv[]){

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, HOST, &addr.sin_addr);

	int sock; 
	int ret; 

	sock = socket(AF_INET, SOCK_STREAM, 0); 

	ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)); 

	int n; 
	char msg[LOG_MSG_SIZE] = "msg from client"; 
	n = write(sock, msg, strlen(msg)); 

	sleep(1); 

	char recv_msg[LOG_MSG_SIZE]; 
	n = read(sock, recv_msg, LOG_MSG_SIZE);
	setInfo("%d  %s", n, recv_msg); 

	close(sock); 

	return n; 
}