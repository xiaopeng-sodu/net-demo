#include <stdio.h>
#include <hiredis/hiredis.h>

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

	printf("%s\n", reply->str); 
	freeReplyObject(reply); 

	redisFree(c); 

	return 0; 
}