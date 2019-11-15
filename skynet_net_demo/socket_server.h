#ifndef socket_server_h
#define socket_server_h

#define SOCKET_ERROR 1
#define SOCKET_CLOSE 2
#define SOCKET_DATA 3
#define SOCKET_ACCEPT 4
#define SOCKET_OPEN 5
#define SOCKET_EXIT 6

struct socket_server;

struct socket_message {
	int id; 
	int opaque; 
	int ud; 
	char *data; 
}; 

struct socket_server * socket_server_create(); 
int socket_server_poll(struct socket_server *ss, struct socket_message * result); 
int socket_server_listen(struct socket_server*ss, int opaque, const char *host, int port, int backlog); 
int socket_server_start(struct socket_server *ss, int opaque, int id); 
int socket_server_send(struct socket_server *ss, int id, char *buffer, int sz);
void socket_server_send_lowpriority(struct socket_server *ss, int id, char *buffer, int sz);
void socket_server_close(struct socket_server *ss, int opaque, int id); 
void socket_server_shutdown(struct socket_server *ss, int opaque, int id); 
int socket_server_connect(struct socket_server *ss, int opaque, const char *host, int port); 
void socket_server_nodelay(struct socket_server *ss, int id); 

void socket_test(struct socket_server * s); 


#endif