#include <stdio.h>
#include <hiredis/hiredis.h>
#include <stdarg.h>

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

	redisContext *c = redisConnect("0.0.0.0", 6379); 

	if (c == NULL || c->err){
		if (c){
			fprintf(stderr, "redisConnect failed, %s\n", c->errstr); 
		}else{
			fprintf(stderr, "cannot allocate redis context\n"); 
		}
	}

	redisReply * reply = redisCommand(c, "set foo 1234"); 
	freeReplyObject(reply); 

	reply = redisCommand(c, "get foo"); 

	setInfo("%s", reply->str); 
	freeReplyObject(reply); 

	reply = redisCommand(c, "lpush list xiaopeng"); 
	freeReplyObject(reply); 

	reply = redisCommand(c, "lrange list 0 -1"); 
	if (reply){
		setInfo("type : %d, elements : %d", reply->type, reply->elements); 
		setInfo("integer : %d, dval : %d",  reply->integer, reply->dval); 
		setInfo("len : %d, str : %s", reply->len, reply->str); 
		int i; 
		for(i=0;i<reply->elements;i++){
			setInfo("str : %s", reply->element[i]->str); 
		}
	}
	freeReplyObject(reply); 

	reply = redisCommand(c, "lpop list"); 
	if(reply){
		setInfo("lpop : %s, len : %d", reply->str, reply->len); 
	}
	freeReplyObject(reply);

	reply = redisCommand(c, "set num 1866666");
	freeReplyObject(reply); 

	reply = redisCommand(c, "get num"); 
	if (reply ){
		setInfo("type : %d, integer : %d, str : %s", reply->type, reply->integer, reply->str); 
	}
	freeReplyObject(reply); 

	redisFree(c); 

	return 0; 
}