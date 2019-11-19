#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define SORT_PATH "./qsort.so"

typedef void (*func)(int data[], int i, int j); 

#define N 10

int data[N] = { 11, 9, 7, 8, 6, 5, 44, 3, 2, 1}; 

int 
main(int argc, char *argv[]){
	void *handle; 
	func sort_func; 

	handle = dlopen(SORT_PATH, RTLD_LAZY); 

	if(!handle){
		fprintf(stderr, "%s\n", dlerror()); 
		exit(0); 
	}

	dlerror(); 


	sort_func = dlsym(handle, "q_sort"); 
	char *error; 
	if((error = dlerror()) != NULL){
		fprintf(stderr, "%s\n", error); 
		exit(0); 
	}

	sort_func(data, 0, N-1); 

	int i; 
	for(i=0;i<N;i++){
		printf("%d ", data[i]); 
	}

	printf("\n");

	dlclose(handle); 

	return 0; 
}