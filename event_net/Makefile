CC?=gcc
OBJECT=test
OBJECT_PATH=.

.PHONY : clean

$(OBJECT_PATH)/$(OBJECT) : $(OBJECT_PATH)/test.c $(OBJECT_PATH)/socket_server.c $(OBJECT_PATH)/listener.c $(OBJECT_PATH)/event.c ./socket.c
	$(CC) -g3 -o $@ $^ -Ievent_net

clean : 
	rm -f $(OBJECT_PATH)/$(OBJECT)
