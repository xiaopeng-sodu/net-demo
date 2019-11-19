#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>

#define LOG_MSG_SIZE 256


char *
str_up(const char *str){
	int len = strlen(str); 
	char *tmp = (char *)malloc(len+1); 
	memcpy(tmp, str, len+1); 
	return tmp; 
}

void
set_info(const char *fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	int n = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 

	char * data; 

	if(n > 0 && n < LOG_MSG_SIZE){
		data = str_up(msg); 
	}else{
		int max_size = LOG_MSG_SIZE; 
		for(;;){
			max_size *= 2; 
			data = malloc(max_size); 
			va_list ap; 
			va_start(ap, fmt); 
			n = vsnprintf(data, max_size, fmt, ap); 
			va_end(ap); 
			if(n < max_size){
				break; 
			}
			free(data); 
		}
	}

	if(n == 0){
		free(data); 
	}

	printf("%s\n", data); 
	free(data); 
	return; 
}
