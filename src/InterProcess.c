/*
 * interprocess.c
 *
 *  Created on: Jan 7, 2011
 *      Author: andy
 */

/*
* Copyright 2010 Andrew M. Leifer  <leifer@fas.harvard.edu>
*
* Interprocess is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Interprocess is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* See <http://www.gnu.org/licenses/>.
*
* For the most up to date version of this software, see:
* http://github.com/samuellab/interprocess
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <tchar.h>

#include "interprocess.h"


#define IP_MAX_MEM_NAME_LENGTH 512

struct SharedMemory_t {
	char name[IP_MAX_MEM_NAME_LENGTH];
	int BufferSize;
	char* ptrToMutex;
	int ReadTimeDelay;
};

struct field_t {
	char name[IP_FIELD_NAME_SIZE];
	char data[IP_FIELD_DATA_CONTAINER_SIZE];
	int size; /* Size of data container used for data*/
};

struct SharedData_t {
	int maxNumFields; /* max number of fields in the shared data */
	int usedFields;
	struct field_t fields[ (IP_BUF_SIZE / sizeof(struct field_t)) - 1];
	HANDLE ghMutex; /*  mutex  indicates who has a lock on the data */
};




/*********************************************************************************/
/*********************************************************************************/
/* 			P R I V A T E      F U N C T I O N S                                 */
/*********************************************************************************/
/*********************************************************************************/

/*
 * Zero a field
 */
int zeroField(struct field_t* field){
	memset( field->name, '\0', sizeof(char) * IP_FIELD_NAME_SIZE );
	memset( field->data, '\0', sizeof(char) * IP_FIELD_DATA_CONTAINER_SIZE );
	return IP_SUCCESS;
}


/*************
 *
 * Create and Destroy Shared Data
 *
 */


/*
 * Create Shared Data.
 * This returns a pointer to a chunk of data that is to be stored in shared memory.
 */
struct SharedData_t* createSharedData(){
	struct SharedData_t* sdata= (struct SharedData_t*) malloc(sizeof(struct SharedData_t));
	if (sdata != NULL){
		sdata->usedFields=0;
		sdata->maxNumFields= (IP_BUF_SIZE / sizeof(struct field_t)) - 1;

		/** Blank out the data with zeros **/
		int k=0;
		for (k = 0; k < sdata->maxNumFields; ++k) {
			/* clear field */
			zeroField( &(sdata->fields[k]) ) ;
		}


		/** Create the mutex **/


	}
	return sdata;

}



/*************
 *  Create and Destroy Shared Memory
 *
 *
 */



/*
 * Start the shared memory host and create a shared memory object.
 * This is to be run on the host process.
 */
SharedMemory_handle ip_CreateSharedMemoryHost(char* name){
	SharedMemory_handle sm = (SharedMemory_handle) malloc(sizeof(struct SharedMemory_t));
	if (sm != NULL){ /* if the object is valid */
		strncpy(  sm->name, name, IP_MAX_MEM_NAME_LENGTH-1);
		sm->name[IP_MAX_MEM_NAME_LENGTH-1]='\0';
		sm->BufferSize=IP_BUF_SIZE;
		sm->ReadTimeDelay=7;
	}

	return sm;
}





/*
 * Start the shared memory client and create a shared memory object.
 * This is to be run on the client process.
 */
SharedMemory_handle ip_CreateSharedMemoryClient(char* name){

}


/*
 *  Close the shared memory.
 *  Returns 0 if success (IP_SUCCESS).
 *  Returns -1 if error (IP_ERROR).
 */
int ip_CloseSharedMemory(SharedMemory_handle sm){

}


/*********************
 *
 *  Get Properties of Shared Memory
 *
 *
 */

/*
 * Get the name of the shared memory. This name is used as the unique identifier.
 */
char* ip_GetSharedMemoryName(SharedMemory_handle sm){

}

/*
 * Get the size of the shared memory.
 *
 */
int ip_GetSharedMemorySize(SharedMemory_handle sm){

}


/*
 *
 * Get shared memory status
 *
 * Returns 0 (IP_SUCCESS) if shared memory exists and is available
 * Returns 1 (IP_BUSY) if shared memory exists but is busy
 * Returns -1 (IP_ERROR) if error
 * Returns -2 (IP_DOESNOTEXIST) if shared memory does not exist
 *
 */
int ip_GetSharedMemoryStatus(SharedMemory_handle sm){

}


/*********************
 *
 *  Read/Write Memory
 *
 *
 */



/*
 * Write a value to shared memory
 *
 * If the name of that value does not exist, it creates that value.
 *
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1
 *  IP_BUSY 1
 *
 */
int ip_WriteValue(SharedMemory_handle sm, char fieldName, void *data, int dataSize){

}


/*
 * Read a value from shared memory
 *
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if shared memory doesn't exist
 *  IP_BUSY 1
 *  IP_DOES_NOT_EXIST -2
 *
 */
int ip_ReadValue(SharedMemory_handle sm, char fieldName, void *data, int dataSize){

}



/*
 * Remove all fields in shared memory
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearAllFields(SharedMemory_handle sm){

}

/*
 *  Clear a single field in shared memory
  * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearField(SharedMemory_handle sm, char fieldName){

}


int main(){
	printf("Welcome!\n");
	struct SharedData_t* sd= createSharedData();
	printf("Created shared data type!\n");
	return IP_SUCCESS;
}


