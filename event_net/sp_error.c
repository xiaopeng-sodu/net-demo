#include <stdarg.h>
#include <stdio.h>

#define LOG_MSG_SIZE 256


void
set_error(const char *fmt, ...){
	va_list ap ; 
	char msg[LOG_MSG_SIZE]; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	perror(msg); 
	va_end(ap); 
}

void
set_info(const char *fmt, ...){
	va_list ap ; 
	char msg[LOG_MSG_SIZE]; 
	va_start(ap, fmt); 
	vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}