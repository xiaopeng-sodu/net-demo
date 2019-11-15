#include <malloc.h>
#include <string.h>

#include "malloc_hook.h"

char * 
skynet_strup(const char *str){
	int len = strlen(str); 
	char *tmp = malloc(len+1); 
	memcpy(tmp, str, len+1); 
	return tmp; 
} 


