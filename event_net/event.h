#ifndef event_h
#define event_h

typedef int (*read_callback)(int epfd, int sock); 
typedef int (*write_callback)(int epfd, int sock); 

typedef struct event {
	int sock; 
	int events; 
	void *ptr ; 

	read_callback recv_handler; 
	write_callback write_handler; 

}event; 


struct event* create_event(int epfd, int sock, read_callback recv_handler, write_callback write_handler); 

#endif