#ifndef sp_queue_h
#define sp_queue_h

#include "spinlock.h"

#define IN_GLOBAL 1

struct skynet_message{
	char * ptr; 
};

struct message_queue{
	struct spinlock_t lock; 
	int handle;
	int head; 
	int tail; 
	int cap; 
	int in_global; 
	int release; 
	struct skynet_message *queue; 
	struct message_queue * next; 
};

void init_global_queue();
void free_global_queue(); 
struct message_queue * create_message_queue(int handle);  
void sp_mq_push(struct message_queue * mq, struct skynet_message *message); 
void sp_mq_pop(struct message_queue* mq, struct skynet_message *message); 
void sp_global_push(struct message_queue *mq); 
struct message_queue * sp_global_pop(); 
int sp_mq_length(struct message_queue * mq); 
int sp_global_length(); 

#endif