#include "sp_queue.h"
#include "sp_error.h"

#include <malloc.h>
#include <string.h>
#include <assert.h>

#define MIN_QUEUE_SIZE  64

struct global_queue {
	struct message_queue * head; 
	struct message_queue * tail; 
	struct spinlock_t lock; 
};

struct global_queue * Q = NULL; 

struct message_queue * 
create_message_queue(int handle){
	struct message_queue * mq = malloc(sizeof(*mq)); 
	memset(mq, 0, sizeof(*mq)); 
	mq->handle = handle; 
	mq->head = 0; 
	mq->tail = 0; 
	mq->cap = MIN_QUEUE_SIZE;
	mq->in_global = 0; 
	mq->release = 0; 
	mq->queue = malloc(sizeof(struct skynet_message) * MIN_QUEUE_SIZE); 
	mq->next = NULL; 
	SPIN_INIT(mq); 

	return mq; 
}

void 
expand_queue(struct message_queue * mq){
	struct skynet_message * new_queue = malloc(sizeof(struct skynet_message) * mq->cap * 2); 
	int i; 
	for(i=0;i<mq->cap;i++){
		new_queue[i] = mq->queue[(mq->head+i)%mq->cap]; 
	}
	mq->head = 0; 
	mq->tail = mq->cap; 
	mq->cap *= 2; 
	free(mq->queue); 
	mq->queue = new_queue;
}

void 
sp_mq_push(struct message_queue * mq, struct skynet_message *message){
	assert(mq); 
	SPIN_LOCK(mq)

	mq->queue[mq->tail++] = *message; 

	if (mq->tail >= mq->cap){
		mq->tail = 0; 
	}

	if(mq->tail == mq->head){
		expand_queue(mq); 
	}

	SPIN_UNLOCK(mq)
}

void 
sp_mq_pop(struct message_queue* mq, struct skynet_message * message){
	assert(mq); 
	SPIN_LOCK(mq)

	*message = mq->queue[mq->head]; 
	mq->head++; 

	if (mq->head >= mq->cap){
		mq->head = 0; 
	}

	if (mq->head == mq->tail){

	}

	SPIN_UNLOCK(mq)
}

void 
sp_global_push(struct message_queue *mq){
	struct global_queue *q = Q; 
	assert(q); 

	SPIN_LOCK(q)

	if (q->head){
		q->tail->next = mq; 
		q->tail = q->tail->next; 
	}else{
		q->tail = q->head = mq; 
	}

	q->tail->next = NULL; 

	SPIN_UNLOCK(q)
}

struct message_queue * 
sp_global_pop(){
	struct global_queue *q = Q; 
	assert(q); 

	SPIN_LOCK(q)

	struct message_queue *mq = q->head; 
	if (mq){
		q->head = q->head->next;
		if(q->head == NULL){
			assert(mq == q->tail); 
			q->tail = NULL;
		}
		mq->next = NULL; 
	}

	SPIN_UNLOCK(q)

	return mq; 
}

int 
sp_global_length(){
	struct global_queue * q = Q; 
	assert(q); 

	SPIN_LOCK(q)

	int len = 0; 
	struct message_queue * mq = q->head; 
	while(mq){
		len += 1; 
		mq = mq->next; 
	}

	SPIN_UNLOCK(q)

	return len; 
}


int 
sp_mq_length(struct message_queue * mq){
	assert(mq); 
	SPIN_LOCK(mq)

	int length ; 
	length = mq->tail - mq->head; 

	if (length < 0){
		length += mq->cap; 
	}

	SPIN_UNLOCK(mq)

	return length; 
}

void
init_global_queue(){
	struct global_queue * q = malloc(sizeof(*q)); 
	memset(q, 0, sizeof(*q)); 

	SPIN_INIT(q)
	q->head = NULL; 
	q->tail = NULL; 
	Q = q; 
}

void
free_global_queue(){
	free(Q); 
}