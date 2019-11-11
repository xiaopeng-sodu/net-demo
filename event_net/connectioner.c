#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "connectioner.h"
#include "socket.h"
#include "event.h"
#include "msg_handler.h"


int 
accept_sock(int sock){
	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr)); 
	int len = sizeof(addr); 

	int new_sock; 
	new_sock = accept(sock, (struct sockaddr*)&addr, sizeof(addr)); 
	if (new_sock < 0){
		fprintf(stderr, "accept new_sock failed \n"); 
		return -1; 
	}

	set_reuseaddr(new_sock); 
	set_nonblocking(new_sock); 

	create_event(new_sock, recv_handler, write_handler); 

}