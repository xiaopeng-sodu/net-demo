#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define LOG_MSG_SIZE 256
#define THREAD_COUNT 4

pthread_mutex_t mutex; 
pthread_cond_t cond; 

static int global_id = 3; 

void
setInfo(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s", msg); 
	va_end(ap); 
}

void*
thread_func(void *argc){
	int thread_id = *((int*)argc); 

	int i; 
	for(i=0;i<10;i++){
		pthread_mutex_lock(&mutex); 

		while (thread_id != global_id){
			pthread_cond_wait(&cond, &mutex); 
		}

		global_id = (global_id+1)%4; 

		FILE * file = fopen("/root/log", "a+"); 
		fprintf(file, "%d ", thread_id); 
		fclose(file); 

		pthread_mutex_unlock(&mutex); 
		pthread_cond_broadcast(&cond); 
	}
	
	return NULL;
}

int
main(int argc, char *argv[]){

	pthread_t tid[4] = {0, 0, 0, 0}; 

	pthread_mutex_init(&mutex, NULL); 
	pthread_cond_init(&cond, NULL); 

	setInfo("server start\n"); 

	int ret; 
	int i; 
	int a[THREAD_COUNT] = {0, 1, 2, 3};
	for(i=0;i<THREAD_COUNT;i++){
		ret = pthread_create(&tid[i], NULL, thread_func, &a[i]); 
		if (ret < 0){
			setInfo("ret : %d", ret); 
			fprintf(stderr, "pthread_create failed: %s\n", strerror(errno)); 
			return -1;
		}
		// setInfo("tid .. %d", tid[i]); 
	}

	for(i=0;i<THREAD_COUNT;i++){
		pthread_join(tid[i], NULL); 
	}

	setInfo("\nstop\n");

	pthread_mutex_destroy(&mutex); 
	pthread_cond_destroy(&cond); 

	return 0; 
}