CC?=gcc
OBJECT=thread_main
OBJECT_PATH=.

.PHONY : clean 

$(OBJECT_PATH)/$(OBJECT): $(OBJECT_PATH)/$(OBJECT).c
	$(CC) -lpthread -g3 -o $@ $^ -Ithread_demo

clean: 
	rm -f $(OBJECT_PATH)/thread_main $(OBJECT_PATH)/thread1