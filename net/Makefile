CC?=gcc
OBJECT=sp_client
OBJECT_PATH = .

.PHONY : clean 

$(OBJECT_PATH)/$(OBJECT) : $(OBJECT_PATH)/$(OBJECT).c 
	$(CC) -g3 -o $@ $^ -Inet

clean : 
	rm -f $(OBJECT_PATH)/sp_server $(OBJECT_PATH)/sp_client