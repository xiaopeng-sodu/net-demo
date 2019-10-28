CC ?= gcc

OBJECT_PATH = .
OBJECT = sp_main

$(OBJECT_PATH)/$(OBJECT) : sp_main.c
	$(CC) -g3 -o $@  $^ -Inet

.PHONY: clean

clean: 
	rm -f $(OBJECT_PATH)/sp_client $(OBJECT_PATH)/sp_main $(OBJECT_PATH)/k_server $(OBJECT_PATH)/k_client


