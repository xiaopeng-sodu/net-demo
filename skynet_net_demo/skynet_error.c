#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>


#include "skynet_error.h"
#include "malloc_hook.h"

#define LOG_MSG_SIZE  256

void
skynet_error(const char *fmt, ...){
	char * data = NULL; 
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	int len = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	va_end(ap); 
	if (len >= 0 && len < LOG_MSG_SIZE){
		data = skynet_strup(msg);
	}else {
		int max_size = LOG_MSG_SIZE; 
		for(;;){
			max_size *= 2; 
			data = malloc(max_size);
			va_start(ap, fmt);  
			len = vsnprintf(data, max_size, fmt, ap); 
			if(len < max_size){
				break; 
			}
			free(data); 
		}
	}

	if (len < 0){
		free(data); 
		perror("vsnprintf failed"); 
		return ; 
	}

	printf("%s\n", data); 
	free(data); 
	return; 
}