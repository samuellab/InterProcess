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
* You should have received a copy of the GNU General Public License
* along with MindControl. If not, see <http://www.gnu.org/licenses/>.
*
* For the most up to date version of this software, see:
* https://github.com/samuellab/interprocess
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <tchar.h>

#include "interprocess.h"


#define MAX_MEM_NAME_LENGTH 512

struct SharedMemory_t {
	char name[MAX_MEM_NAME_LENGTH];
	int BufferSize;
	char* ptrToMutex;
};




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
	SharedMemory_handle sm = malloc(sizeof(struct SharedMemory_t));
	if (sm != NULL){ /* if the object is valid */
		strncpy( sm.name, name, MAX_MEM_NAME_LENGTH-1);
		sm.name[MAX_MEM_NAME_LENGTH-1]='\0';
		sm.BufferSize=BUF_SIZE;
	}

}





/*
 * Start the shared memory client and create a shared memory object.
 * This is to be run on the client process.
 */
SharedMemory_handle ip_CreateSharedMemoryClient(char* name);


/*
 *  Close the shared memory.
 *  Returns 0 if success (IP_SUCCESS).
 *  Returns -1 if error (IP_ERROR).
 */
int ip_CloseSharedMemory(SharedMemory_handle sm);


/*********************
 *
 *  Get Properties of Shared Memory
 *
 *
 */

/*
 * Get the name of the shared memory. This name is used as the unique identifier.
 */
char* ip_GetSharedMemoryName(SharedMemory_handle sm);

/*
 * Get the size of the shared memory.
 *
 */
int ip_GetSharedMemorySize(SharedMemory_handle sm);


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
int ip_GetSharedMemoryStatus(SharedMemory_handle sm);


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
int ip_WriteValue(SharedMemory_handle sm, char fieldName[16], void *data, int dataSize);


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
int ip_ReadValue(SharedMemory_handle sm, char fieldName[16], void *data, int dataSize);



/*
 * Remove all fields in shared memory
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearAllFields(SharedMemory_handle sm);

/*
 *  Clear a single field in shared memory
  * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearField(SharedMemory_handle sm, char fieldName);

