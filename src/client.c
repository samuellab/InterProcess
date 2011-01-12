/*
 * client.c
 *
 *  Created on: Jan 11, 2011
 *      Author: andy
 */



#include <stdio.h>

#include "interprocess.h"

int main(){
	printf("Welcome!\n");

	SharedMemory_handle mySharedMemClient = ip_CreateSharedMemoryClient("YourMama1");
	if(mySharedMemClient == NULL){
		printf("creating shared memory failed.\n");
	} else {
		printf("Success!\n");
	}
	ip_CloseSharedMemory(mySharedMemClient);
	printf("Destroyed Shared Memory!\n");
	return IP_SUCCESS;
}

