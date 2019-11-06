#include <hiredis/hiredis.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

#define HOST "0.0.0.0"
#define PORT 6379
#define LOG_MSG_SIZE 256

void 
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE]; 
	va_list ap ; 
	va_start(ap, fmt); 
	int len = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}

int 
main(int argc, char *argv[]){

	redisAsyncContext * ctx = redisAsyncConnect(HOST, PORT); 
	if (ctx->err){
		setInfo("error : %s", ctx->errstr); 
		exit(1); 
	}

	

	return 0; 
}