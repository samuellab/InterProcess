/*
 * interprocess.h
 *
 *  Created on: Jan 7, 2011
 *      Author: andy
 *
 * InterProcess is a fast, compact library for sharing fields of data between two processes
 * on Windows. It exposes only a very simple interface and abstracts away all of the
 * underlying mechanics.
 *
 * Under the hood, InterProcess avoids conflicts using mutex and uses the Windows
 * Named Shared Memory to share fields of data.
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
* https://github.com/samuellab/interprocess
*
*/


#ifndef INTERPROCESS_H_
#define INTERPROCESS_H_





/** Hard code in the buffer sizes **/
#define IP_BUF_SIZE 256000
#define IP_FIELD_NAME_SIZE 32
#define IP_FIELD_DATA_CONTAINER_SIZE 512

/** Return Values **/
#define IP_SUCCESS 0
#define IP_ERROR -1
#define IP_BUSY 1
#define IP_DOES_NOT_EXIST -2
#define IP_NO_MORE_ROOM -3



/*************
 *  Create and Destroy Shared Memory
 *
 *
 */

/*
 * This is the SharedMemory object.
 * It is the base object for this library.
 *
 */
typedef struct SharedMemory_t *SharedMemory_handle;




/*
 * Start the shared memory host and create a shared memory object.
 * This is to be run on the host process.
 */
SharedMemory_handle ip_CreateSharedMemoryHost(char* name);



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


/*
 * Get The Refactory Period (Time Delay) for reading from shared memory
 * time_ms stores the time delayin ms
 *
 * Both reading or writing a value locks the shared memory. By default, when a client
 * successfully reads a value from shared memory, the client is forced to sleep for a specified time
 * (default is 7ms). I call this the Read Time Delay.
 *
 * The Read Time Delay gives another process time to access the shared memory, and prevents an overly
 * aggressive client from reading in a loop and hogging the lock.
 *
 * Note.. this implicitly prioritizes writing over reading.
 *
 * To turn off the Read Time Delay, set the time to zero.
 *
 * Returns IP_SUCCESS 0
 * or IP_ERROR -1
 *
 */
int ip_GetSharedMemoryReadRefractoryPeriodTimeDelay(SharedMemory_handle sm, int* time_ms);

/*
 * Set the number of milliseconds of the Read Time Delay.
 *
 * Both reading or writing a value locks the shared memory. By default, when a client
 * successfully reads a value from shared memory, the client is forced to sleep for a specified time
 * (default is 7ms). I call this the Read Time Delay.
 *
 * The Read Time Delay gives another process time to access the shared memory, and prevents an overly
 * aggressive client from reading in a loop and hogging the lock.
 *
 * Note.. this implicitly prioritizes writing over reading.
 *
 * To turn off the Read Time Delay, set the time to zero.
 * Returns IP_SUCCESS 0
 * or IP_ERROR -1
 */
int ip_SetSharedMemoryReadRefractoryPeriodTimeDelay(SharedMemory_handle sm, int time_ms);



int ip_SetSharedMemoryLockWaitTime(SharedMemory_handle sm, int time_ms);

int ip_GetSharedMemoryLockWaitTime(SharedMemory_handle sm);

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
int ip_WriteValue(SharedMemory_handle sm, char* fieldName, void *data, int dataSize);


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
int ip_ReadValue(SharedMemory_handle sm, char* fieldName, void *data);



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
int ip_ClearField(SharedMemory_handle sm, char* fieldName);



/**************************************************************/
/**************************************************************/
/** PRIVATE FUNCTIONS!! DONT FORGET TO DELETE THESE.. FOR DEVEL ONLY **/
/**************************************************************/
/**************************************************************/
/*
 *  Tries to acquire a lock by waiting until the mutex is released.
 *  The function will wait the amount of time specified in the SharedMemory object
 *  (the default is 4ms).
 *
 *  This function returns
 *  0 IP_SUCCESS
 *  1 IP_BUSY
 *
 *  Don't forget to call ReleaseLock()
 */
int AcquireLock(SharedMemory_handle sm);

/*
 * Tries to release the mutex. Returns IP_ERROR or IP_SUCCESS;
 */
int ReleaseLock(SharedMemory_handle sm)	;

#endif /* INTERPROCESS_H_ */

