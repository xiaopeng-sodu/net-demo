#ifndef sp_server_h
#define sp_server_h

void create_socket_server(); 
int init_socket_server(); 
int socket_server_poll(); 

int recv_info(int id); 
int write_info(int id); 

void free_socket_server(); 

int do_bind(int sock); 
int do_accept(); 

void add_sock_epoll(int sock); 


#endif