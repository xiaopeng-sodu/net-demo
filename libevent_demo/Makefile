CC=gcc
OBJECT=event_timer
OBJECT_PATH=.
SHARED=-fPIC --shared

.PHONY : clean

SRC = event_timer.c



$(OBJECT_PATH)/$(OBJECT) : $(foreach v, $(SRC), $(OBJECT_PATH)/$(v)) 
	$(CC) -g3 -o $@ $^ -I/usr/local/include -L/usr/local/lib -levent -ldl

$(OBJECT_PATH)/qsort.so : $(OBJECT_PATH)/sort.c
	$(CC) $(SHARED) -g3 -o $@ $^ -Ilibevent_demo 

clean: 
	rm -f $(OBJECT_PATH)/$(OBJECT)