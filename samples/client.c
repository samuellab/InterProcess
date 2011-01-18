/*
 * client.c
 *
 *  Created on: Jan 11, 2011
 *      Author: andy
 */



#include <stdio.h>

#include "../src/interprocess.h"

int main(){
	printf("Welcome!\n");

	SharedMemory_handle mySharedMemClient = ip_CreateSharedMemoryClient("YourMama1");
	if(mySharedMemClient == NULL){
		printf("creating shared memory failed.\n");
	} else {
		printf("Looking for value int_life\n");
		int val=0;
		int ret=ip_ReadValue(mySharedMemClient,"int_life",(void *) &val);
		if (ret==IP_SUCCESS){
			printf("Success!\n value is %d\n",val);
		} else {
			printf("ERROR! %d\n",ret);
		}
		printf("Looking for value int_val\n");
		val=0;
		ret=ip_ReadValue(mySharedMemClient,"int_val",(void *) &val);
		if (ret==IP_SUCCESS){
			printf("Success!\n value is %d\n",val);
		} else {
			printf("ERROR! %d\n",ret);
		}

	}
	ip_CloseSharedMemory(mySharedMemClient);
	printf("Destroyed Shared Memory!\n");
	return IP_SUCCESS;
}

