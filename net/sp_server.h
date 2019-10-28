#ifndef sp_server_h
#define sp_server_h

void create_socket_server(); 
int init_socket_server(); 
void socket_server_poll(); 

int recv_info(int sock); 
int write_info(int sock); 

void free_socket_server(); 

int do_bind(int sock); 
int do_accept(); 


#endif