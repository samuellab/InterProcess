/*
 * host.c
 *
 *  Created on: Jan 11, 2011
 *      Author: andy
 */

#include <conio.h>
#include <stdio.h>

#include "../src/interprocess.h"

int main(){
	printf("Welcome!\n");
	SharedMemory_handle mySharedMem = ip_CreateSharedMemoryHost("YourMama1");
	printf("Created shared memory host!\n");
	printf("Storing the value 47 into variable name int_life..\n");
	int val=47;

	int ret=ip_WriteValue(mySharedMem,"int_life",(void *) &val, sizeof(int));
	printf("Storing the value 100 into variable name int_val..\n");
	int val2=100;
	int ret2=ip_WriteValue(mySharedMem,"int_val",(void *) &val2, sizeof(int));
	if (ret==IP_SUCCESS){
		printf("stored!\n");

	} else {
		printf("ERROR! %d\n", ret);
	}

	printf("Press enter to acquire lock!\n");
	getch();
	if (AcquireLock(mySharedMem)==IP_SUCCESS){
		printf("Acquired Lock!");
		printf("Press any key to release lock and clear int_life\n");
		getch();
		ip_ClearField(mySharedMem,"int_life");
		printf("int_life is cleared\n");
		if (ReleaseLock(mySharedMem)==IP_SUCCESS){
			printf("Released!\n");
		}else{
			printf("Could not release.\n");
		}

	} else{
		printf("Couldn't acquire Lock");
	}

	printf("Press any key to close\n");
	getch();
	ip_CloseSharedMemory(mySharedMem);
	printf("Destroyed Shared Memory!\n");

	return IP_SUCCESS;
}


