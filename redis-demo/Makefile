CC?=gcc
OBJECT=redis
OBJECT_PATH=.

$(OBJECT_PATH)/$(OBJECT): $(OBJECT_PATH)/$(OBJECT).c
	$(CC) -g3 -o $@  $^ -Iredis-demo  -lhiredis

.PHONY : clean

clean : 
	rm -f $(OBJECT_PATH)/$(OBJECT)