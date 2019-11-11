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
event_read_callback(int epfd, int sock){
	int n = recv_handle(sock); 
	return n; 
}

int 
event_write_callback(int epfd, int sock){
	int n = write_handle(sock); 
	return n; 
}

int 
connection_callback(int epfd, int sock){
	struct event *ev = create_event(epfd, sock, event_read_callback, event_write_callback); 

	return 0; 
}