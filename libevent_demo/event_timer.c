#include <stdio.h>
#include <time.h>

#include <event2/event.h>

void
onTime(int sock, short events, void *argc){
	printf("game over\n"); 

	struct timeval tv;
	tv.tv_sec = 1; 
	tv.tv_usec = 0; 

	event_add((struct event*)argc, &tv); 
}


int 
main(int argc, char *argv){

	event_init(); 

	struct event evtime; 
	evtimer_set(&evtime, onTime, &evtime); 

	struct timeval tv; 
	tv.tv_sec = 1; 
	tv.tv_usec = 0; 

	event_add(&evtime, &tv); 

	event_dispatch(); 

	return 0; 
}