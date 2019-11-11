#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "error.h"

int 
recv_handler(int sock){
	int n; 
	char msg[256]; 
	n = read(sock, msg, sizeof(msg)); 
	if (n < 0){
		switch(errno){
			case EINR: 
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
write_handler(int sock){
	int n ; 
	char msg[256] = "msg--from--server"; 
	n = write(sock, msg, strlen(msg)); 
	if (n < 0){
		switch(errno){
			case EINR: 
			case EAGAIN:
			default:{

			}
		}
	}

	return n; 
}