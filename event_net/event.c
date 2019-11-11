#include <malloc.h>
#include <string.h>

#include "event.h"
#include "epoll.h"
#include "socket_server.h"

struct event*
create_event( int sock, 
			    read_callback recv_handler, 
				write_callback write_handler )
{
	struct event *e = malloc(sizeof(*e)); 
	memset(e, 0, sizeof(*e)); 

	e->sock = sock; 
	e->recv_handler = recv_handler ; 
	e->write_handler = write_handler; 

	sp_add(S->epfd, sock, (void*)e);    // add event to epoll
}