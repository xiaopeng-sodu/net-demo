CC ?= gcc
OBJECT = sp_client
OBJECT_PATH = .

$(OBJECT_PATH)/$(OBJECT) : $(OBJECT_PATH)/sp_client.c 
	$(CC) -g3 -o $@  $^ -Inet_demo1

.PHONY: clean

clean:
	rm -f $(OBJECT_PATH)/sp_server $(OBJECT_PATH)/sp_client $(OBJECT_PATH)/server $(OBJECT_PATH)/client
