#ifndef sp_error_h
#define sp_error_h

#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>
#include <error.h>
#include <assert.h>
#include <string.h>

#define LOG_MSG_SIZE 256

static void 
setError(const char *fmt, ...){
	// char * msg = malloc(LOG_MSG_SIZE); 
	// memset(msg, 0, LOG_MSG_SIZE); 
	char msg[LOG_MSG_SIZE]; 
	memset(msg, 0, LOG_MSG_SIZE); 
	assert(msg); 
	va_list ap ; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 
	perror(msg); 
	// free(msg); 
}


static void 
setInfo(const char *fmt, ...){
	// char *msg = malloc(LOG_MSG_SIZE);
	char msg[LOG_MSG_SIZE];  
	memset(msg, 0, LOG_MSG_SIZE); 
	assert(msg); 
	va_list ap ; 
	va_start(ap , fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 
	printf("%s\n", msg); 
	// free(msg); 
}

#endif