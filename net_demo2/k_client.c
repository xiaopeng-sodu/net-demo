#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>


#define HOST "127.0.0.1"
#define PORT 8888
#define LOG_MSG_SIZE 256

void setInfo(const char * fmt, ...);

int 
main(int argc, char *argv[]){

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr); 

	int sock; 
	int ret; 

	sock = socket(AF_INET, SOCK_STREAM, 0); 
	setInfo("socket, sock : %d", sock); 

	ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	setInfo("connect, ret : %d", ret);

	char msg[LOG_MSG_SIZE] = "client msg";
	int n; 
	n = write(sock, msg, LOG_MSG_SIZE); 
	setInfo("n : %d", n); 

	sleep(1);

	memset(msg, 0, LOG_MSG_SIZE); 
	n = read(sock, msg, LOG_MSG_SIZE); 
	setInfo("n : %d, msg: %s", n, msg); 

	n = write(sock, msg, LOG_MSG_SIZE); 
	setInfo("n : %d", n); 

	sleep(1);

	memset(msg, 0, LOG_MSG_SIZE); 
	n = read(sock, msg, LOG_MSG_SIZE); 
	setInfo("n : %d, msg: %s", n, msg); 


	close(sock); 

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