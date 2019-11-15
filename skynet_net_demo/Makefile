CC=gcc
OBJECT=test
OBJECT_PATH=.

.PHONY : clean

SRC = test.c socket_server.c skynet_socket.c skynet_error.c malloc_hook.c


$(OBJECT_PATH)/$(OBJECT) : $(foreach v, $(SRC), $(OBJECT_PATH)/$(v))
	$(CC) -g3 -o $@ $^ -Iskynet_net_demo

clean:
	rm -f $(OBJECT_PATH)/$(OBJECT)