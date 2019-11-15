#include <stdio.h>
#include <pthread.h>


#include "skynet_socket.h"

#define HOST "0.0.0.0"
#define PORT 8888


int 
main(int argc, char *argv[]){

	int opaque =1 ; 

	skynet_socket_create(); 
	int id = skynet_socket_listen(opaque, HOST, PORT); 
	skynet_socket_start(opaque, id); 
	for(;;){
		skynet_socket_poll(); 
	}

	return 0; 
}