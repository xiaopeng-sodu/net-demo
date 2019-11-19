#include <stdio.h>


void
swap(int data[], int i, int j){
	int c = data[i]; 
	data[i] = data[j]; 
	data[j] = c; 
}

void
q_sort(int data[], int l, int u){
	if(l < u){
		int m = l; 
		int i;
		for(i=l+1;i<=u;i++){
			if(data[i] < data[l]){
				swap(data, ++m, i); 
			}
		}

		swap(data, m, l); 

		if(l < m)
			q_sort(data, l, m-1); 

		if(u > m)
			q_sort(data, m+1, u); 
	}
}