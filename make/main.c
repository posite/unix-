#include "ipc.h"
#include <stdio.h>

int main(){
	printf("shared memory start \n\n");
	shmtest();
	printf("\nshared memory end \n");

	return 0;
}
