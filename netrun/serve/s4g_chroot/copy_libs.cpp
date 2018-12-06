#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

void *fn(void *arg) {printf("This is a thread\n"); return 0;}

int main() {
	printf("Yo.\n");
	std::cout<<"Sup?\n"<<cos(1.0)<<" be the password.  Peace.\n";
	pthread_t pt;
	pthread_create(&pt,0,fn,0);
	return 0;
}
