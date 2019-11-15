#ifndef skynet_socket_h
#define skynet_socket_h

#define SKYNET_SOCKET_TYPE_DATA 1
#define SKYNET_SOCKET_TYPE_ACCEPT 2
#define SKYNET_SOCKET_TYPE_CLOSE 3
#define SKYNET_SOCKET_TYPE_ERROR 4
#define SKYNET_SOCKET_TYPE_CONNECT 5
#define SKYNET_SOCKET_TYPE_WARNING 6

struct skynet_socket_message{
	int type; 
	int id; 
	int ud; 
	char *buffer; 
}; 

void skynet_socket_create(); 
int skynet_socket_listen(int opaque, const char *host, int port); 
int skynet_socket_start(int opaque, int id); 
int socket_server_poll(); 
int skynet_socket_send(int id, void * buffer, int sz); 
void skynet_socket_send_priority(int id, void *buffer, int sz); 
void skynet_socket_close(int opaque,  int id); 
void skynet_socket_shutdown(int opaque, int id); 

#endif
