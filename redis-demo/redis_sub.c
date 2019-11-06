#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdarg.h>

#define HOST "0.0.0.0"
#define PORT  6379 
#define LOG_MSG_SIZE 256

void
setInfo(const char * fmt, ...){
	char msg[LOG_MSG_SIZE];
	va_list ap ; 
	va_start(ap,fmt); 
	int len = vsnprintf(msg, LOG_MSG_SIZE, fmt, ap); 
	printf("%s\n", msg); 
	va_end(ap); 
}

void
processReply(redisReply * reply){
	redisReply * subreply = NULL; 

	if (reply != NULL && reply->elements == 3){
		setInfo("elements : %d", reply->elements); 
		int i; 
		for(i=0;i<reply->elements;i++){
			setInfo("%s", reply->element[i]->str); 
		}
	}
}

int 
main(int argc, char *argv []){

	redisContext * ctx = redisConnect(HOST, PORT); 

	if (ctx == NULL || ctx->err == 1){
		if (ctx){
			fprintf(stderr, "redisConnect failed : %s\n", strerror(ctx->errstr)); 
			return -1; 
		}else{
			fprintf(stderr, "redisConnect failed.."); 
			return -1; 
		}
	}

	redisReply * reply = redisCommand(ctx, "subscribe redischat"); 
	// setInfo("type : %d, str : %s, len: %d, integer : %d, elements : %d", \
	// 	reply->type, reply->str, reply->len, reply->elements); 
	freeReplyObject(reply); 


	while( redisGetReply(ctx, (void**)&reply) == REDIS_OK ){
		processReply( reply ); 
		freeReplyObject( reply ); 
	}

	redisFree(ctx); 

	return 0; 
}