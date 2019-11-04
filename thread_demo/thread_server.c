#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define THREAD_COUNT 4
#define LOG_MSG_SIZE 256

pthread_mutex_t mutex; 
pthread_cond_t cond; 

static int global_id = 0; 

void
setInfo(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s \n", msg); 
	va_end(ap); 
}

void*
thread_func(void *argc){
	int thread_id = *((int*)argc);

	int i; 
	for(i=0;i<10;i++){
		pthread_mutex_lock(&mutex); 

		while(thread_id != global_id){
			pthread_cond_wait(&cond, &mutex); 
		}

		global_id = global_id%THREAD_COUNT; 

		FILE * file = fopen("/root/log", O_CREAT | O_APPEND, 0644);
		fprintf(file, "%d ", thread_id); 
		fclose(file);  

		pthread_mutex_unlock(&mutex); 

		pthread_cond_broadcast(&cond); 
	}
	return NULL; 
}

int 
main(int argc, char *argv[]){

	pthread_t tid[THREAD_COUNT]; 

	pthread_mutex_init(&mutex); 
	pthread_cond_init(&cond); 

	int a[THREAD_COUNT] = {1, 2, 3, 4}; 

	int i;
	int ret;  
	for(i=0;i<THREAD_COUNT;i++){
		ret = pthread_create(&tid[i], NULL, thread_func, &a[i]); 
		if (ret != 0){
			fprintf(stderr, "pthread_create failed : %s", strerror(errno)); 
			return -1; 
		}
	}

	for(i=0;i<THREAD_COUNT;i++){
		pthread_join(tid[i], NULL); 
	}

	pthread_mutex_destroy(&mutex); 
	pthread_cond_destroy(&cond); 

	return -1; 
}