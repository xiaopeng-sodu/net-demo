#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>


int
set_reuseaddr(int sock){
	int reuse = 1; 
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1){
		return -1;
	}
	return 0; 
}

void
set_nonblocking(int sock){
	int flags = fcntl(sock, F_GETFL, 0); 
	flags |= O_NONBLOCK; 
	fcntl(sock, F_SETFL, flags); 
}