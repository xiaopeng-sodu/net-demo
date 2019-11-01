#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define LOG_MSG_SIZE 256
#define THREAD_COUNT 1

pthread_mutex_t mutex; 
pthread_cond_t cond; 

static int n =1;

void
setInfo(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	memset(msg, 0, LOG_MSG_SIZE); 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s", msg); 
	va_end(ap); 
}

void *
thread_func(void *argc){

	int thread_id = *((int*)argc); 

	int i; 
	for(i=0;i<5;i++){
		pthread_mutex_lock(&mutex); 

		while(thread_id != n){
			pthread_cond_wait(&cond, &mutex); 
		}

		n = (n+1)%THREAD_COUNT; 

		pthread_mutex_unlock(&mutex);
		pthread_cond_broadcast(&cond); 
	}
	  
}

void *
thread_file_func(void *argc){
	pthread_mutex_lock(&mutex); 

	char msg[] = "msg from server"; 

	int global_id = 10; 
	FILE *file = fopen("/root/log", "w+"); 
	fprintf(file, "%d", global_id); 

	fclose(file); 	

	pthread_mutex_unlock(&mutex); 
}

int 
main(int argc, char * argv[]){
	pthread_t pid[THREAD_COUNT]; 

	pthread_mutex_init(&mutex, NULL); 
	pthread_cond_init(&cond, NULL); 

	int ret ; 
	int i; 
	// int a[THREAD_COUNT] = {1, 2, 3, 4}; 

	for (i=0;i<THREAD_COUNT;i++){
		ret = pthread_create(&pid[i], NULL, thread_file_func, NULL); 
		if (ret == -1){
			fprintf(stderr, "pthread_create failed : %s\n", strerror(errno)); 
			return -1; 
		}
	}

	for(i=0;i<THREAD_COUNT;i++){
		pthread_join(pid[i], NULL); 
	}

	pthread_mutex_destroy(&mutex); 
	pthread_cond_destroy(&cond); 

	return 0; 
}