#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <errno.h>

#include "sp_error.h"
#include "msg_handler.h"

void 
force_close(int sock){
	close(sock); 
}

int 
recv_handle(int sock){
	int n; 
	char msg[256]; 
	n = read(sock, msg, sizeof(msg)); 
	set_info("n : %d, msg : %s", n, msg); 
	if (n < 0){
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN: 
				break; 
			default:{
				force_close(sock); 
				return -1; 
			} 

		}
	}

	if (n == 0){
		force_close(sock); 
		return -1; 
	}

	return n; 
}


int 
write_handle(int sock){
	int n ; 
	char msg[256] = "msg--from--server"; 
	n = write(sock, msg, strlen(msg)); 
	if (n < 0){
		switch(errno){
			case EINTR: 
				break; 
			case EAGAIN:
				break; 
			default:{
				return -1; 
			}
		}
	}

	return n; 
}