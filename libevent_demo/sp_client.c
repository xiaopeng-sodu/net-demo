#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <netdb.h>
#include <stdarg.h>

#define HOST "0.0.0.0"
#define PORT 8888
#define LOG_MSG_SIZE 256


int 
main(int argc, char *argv[]){

	int conn_sock; 

	conn_sock = socket(AF_INET, SOCK_STREAM, 0); 

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, HOST, &addr.sin_addr); 

	int status ; 
	status = connect(conn_sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if(status != 0){
		return -1; 
	}

	char msg[LOG_MSG_SIZE] = "msg from client"; 
	int n; 
	int len = strlen(msg); 
	n = write(conn_sock, msg, len+1); 
	if(n < 0){
		switch(errno){
			case EINTR:
				break;
			case EAGAIN:	
				break;
			default:
				close(conn_sock); 
				return -1;
		}
	}

	memset(msg, 0, sizeof(msg)); 

	n = read(conn_sock, msg, LOG_MSG_SIZE); 
	if (n < 0){
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN : 
				break; 
			default: 
				close(conn_sock); 
				return -1; 
		}
	}

	printf("%s\n", msg); 

	close(conn_sock); 

	return 0; 
}