#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8888
#define HOST "127.0.0.1"
#define LOG_MSG_SIZE 256

int
main(int argc, char *argv[]){

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(PORT); 
	inet_aton(HOST, &addr.sin_addr);

	int sock ; 
	int ret; 
	int len;

	sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock < 0){
		fprintf(stderr, "socket failed , sock: %d", sock); 
		return -1; 
	}

	ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if (ret != 0){
		fprintf(stderr, "bind socket failed, ret: %d", ret); 
		return -1; 
	}

	ret = listen(sock, 5); 
	if (ret != 0){
		fprintf(stderr, "listen socket failed, ret : %d", ret);
		return -1; 
	}

	struct sockaddr_in c_addr; 
	memset(&c_addr, 0, sizeof(c_addr)); 
	len = sizeof(c_addr); 

	int new_sock; 
	new_sock = accept(sock, (struct sockaddr*)&c_addr, &len); 
	if(new_sock < 0){
		fprintf(stderr, "accept socket failed, new_sock: %d", new_sock); 
		return -1; 
	}

	printf("new_sock : %d", new_sock); 

	int n; 
	char msg[LOG_MSG_SIZE]; 
	n = read(new_sock , msg, LOG_MSG_SIZE); 

	printf("msg : %s", msg); 

	return 0; 
}