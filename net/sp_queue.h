#ifndef sp_queue_h
#define sp_queue_h

struct message_queue; 
struct skynet_message{
	char * ptr; 
};

void init_global_queue();
void free_global_queue(); 
struct message_queue * create_message_queue(int handle);  
void sp_mq_push(struct message_queue * mq, struct skynet_message *message); 
void sp_mq_pop(struct message_queue* mq, struct skynet_message *message); 
void sp_global_push(struct message_queue *mq); 
struct message_queue * sp_global_pop(); 
int sp_mq_length(struct message_queue * mq); 

#endif