CC=gcc
OBJECT=test
OBJECT_PATH=.

.PHONY : clean

src = test.c socket_server.c listener.c event.c socket.c connectioner.c msg_handler.c sp_error.c

$(OBJECT_PATH)/$(OBJECT) : $(foreach v, $(src), ./$(v))
	$(CC) -g3 -o $@ $^ -Ievent_net

clean : 
	rm -f $(OBJECT_PATH)/$(OBJECT)
