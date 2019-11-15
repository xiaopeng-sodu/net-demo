#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "socket_server.h"
#include "skynet_error.h"
#include "skynet_socket.h"

#define BACKLOG 50


struct socket_server * SOCKET_SERVER = NULL; 

void
skynet_socket_create(){
	SOCKET_SERVER = socket_server_create(); 
	assert(SOCKET_SERVER); 
	socket_test(SOCKET_SERVER); 
}

int 
skynet_socket_listen(int opaque, const char *host, int port){
	int id = socket_server_listen(SOCKET_SERVER, opaque, host, port, BACKLOG); 

	return id; 
}

int 
skynet_socket_start(int opaque, int id){
	int new_id = socket_server_start(SOCKET_SERVER, opaque, id); 

	return new_id; 
}

int 
skynet_socket_send(int id, void * buffer, int sz){
	int n  = socket_server_send(SOCKET_SERVER, id, buffer, sz);

	return n; 
}

void 
skynet_socket_send_priority(int id, void *buffer, int sz){
	socket_server_send_lowpriority(SOCKET_SERVER, id,  buffer, sz); 
}

void 
skynet_socket_close(int opaque,  int id){
	socket_server_close(SOCKET_SERVER, opaque, id); 
}

void 
skynet_socket_shutdown(int opaque, int id){
	socket_server_shutdown(SOCKET_SERVER, opaque, id); 
}

static 
forward_message(int type, bool padding, struct socket_message * result){
	int id = result->id; 
	int opaque = result->opaque; 
	int ud = result->ud; 
	char *data = result->data; 

	if (data){
		skynet_error("id : %d, opaque: %d, ud : %d, data : %s", id, opaque, ud, data); 
	}else{
		skynet_error("id : %d, opaque: %d, ud : %d", id, opaque, ud); 	
	}
}


int 
skynet_socket_poll(){
	struct socket_server *ss = SOCKET_SERVER; 
	assert(ss); 

	struct socket_message result; 

	int type = socket_server_poll(ss, &result); 
	switch(type){
		case SOCKET_EXIT: 
			return 0; 
		case SOCKET_ERROR : 
			forward_message(SKYNET_SOCKET_TYPE_ERROR, true, &result); 
			break;
		case SOCKET_OPEN : 
			forward_message(SKYNET_SOCKET_TYPE_CONNECT, true, &result); 
			break; 
		case SOCKET_ACCEPT: 
			forward_message(SKYNET_SOCKET_TYPE_ACCEPT, true, &result);
			break; 
		case SOCKET_CLOSE: 
			forward_message(SKYNET_SOCKET_TYPE_CLOSE, false, &result); 
			break;
		case SOCKET_DATA: 
			forward_message(SKYNET_SOCKET_TYPE_DATA, true, &result); 
			break; 
		default : 
			skynet_error("default");
			return -1; 
	}

	return 1; 
}
