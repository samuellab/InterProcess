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
#include <string.h>

#include "interprocess.h"


#define IP_MAX_MEM_NAME_LENGTH 512

/** Time in ms a client thread should wait for a mutex to become available **/
#define IP_WAIT_FOR_MUTEX_AVAILABILITY 4

/*
 * A field is the term I use for a variable that is stored in shared memory.
 */
struct field_t {
	char name[IP_FIELD_NAME_SIZE];
	char data[IP_FIELD_DATA_CONTAINER_SIZE];
	int size; /* Size of data container used for data*/
};

/*
 * Shared data type that will ultimately be stored in shared memory
 */
struct SharedData_t {
	int maxNumFields; /* max number of fields in the shared data */
	int usedFields;
	struct field_t fields[ (IP_BUF_SIZE / sizeof(struct field_t)) - 1];
};


/*
 * Local object that provides information about the shared memory.
 */
struct SharedMemory_t {

	/* Properties of the Shared Memory */
	char name[IP_MAX_MEM_NAME_LENGTH];
	int BufferSize;
	int ReadTimeDelay;

	/* Application Level Data */
	struct SharedData_t* sd; /* pointer to the location of the shared data  (eventually will be pBuf)*/

	/* Windows Level File Mapping **/
	HANDLE hMapFile; /* handle to mapped file of the shared memroy */
	LPCTSTR pBuf; /* pointer to location of shared memory */

	/* Mutex Locking Properties */
	HANDLE ghMutex; /*  mutex  indicates who has a lock on the data */
	DWORD lockWaitTime; /* number of ms to wait for lock */

};



/*********************************************************************************/
/*********************************************************************************/
/* 			P R I V A T E      F U N C T I O N S                                 */
/*********************************************************************************/
/*********************************************************************************/

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
int ReleaseLock(SharedMemory_handle sm);

/*
 * Destroy Shared memory and deallocate memory
 */
int destroySharedMemoryObj(SharedMemory_handle sm);

/*
 * Zero a field
 */
int zeroField(struct field_t* field);


/*
 * Create Shared Memory Object
 *
 */
SharedMemory_handle createSharedMemoryObj(char* name, HANDLE hMapFile, LPCTSTR pBuf);

/*
 * Destroy Shared memory obj and deallocate memory
 */
int destroySharedMemoryObj(SharedMemory_handle sm);

/*
 * Create Shared Data chunk.
 * This returns a pointer to a chunk of data that is to be stored in shared memory.
 */
struct SharedData_t* createSharedData();

/*
 * destroy Shared Data chunkc
 */
int destroySharedData(SharedData_t* sd);

/*
 * Do a simple sanity check on the Shared Data Struct
 */
int verifySharedDataStruct(SharedData_t* sd);



/*
 * Zero a field
 */
int zeroField(struct field_t* field){
	memset( field->name, '\0', sizeof(char) * IP_FIELD_NAME_SIZE );
	memset( field->data, '\0', sizeof(char) * IP_FIELD_DATA_CONTAINER_SIZE );
	return IP_SUCCESS;
}




/*
 * Create Shared Memory Object
 *
 */
SharedMemory_handle createSharedMemoryObj(char* name, HANDLE hMapFile, LPCTSTR pBuf){
	/* Create Local Shared Memory Object to Store Information */
	SharedMemory_handle sm= (SharedMemory_handle) malloc(sizeof(struct SharedMemory_t));

	/* Initialize the Local Shared Memory Object */
	strncpy(  sm->name, name, IP_MAX_MEM_NAME_LENGTH-1);
	sm->name[IP_MAX_MEM_NAME_LENGTH-1]='\0';
	sm->BufferSize=IP_BUF_SIZE;
	sm->ReadTimeDelay=7;
	sm->pBuf=pBuf;
	sm->hMapFile=hMapFile;
	sm->sd=NULL;

	/** Create Mutex for Shared Memory **/
    // Create a mutex with no initial owner
	sm->ghMutex=NULL;
	sm->lockWaitTime=4;

	/* give the mutex the same name as the memory object but with the prefix "mutex_" */
	char mutex_name[IP_MAX_MEM_NAME_LENGTH+5];
	strcpy(mutex_name,"mutex_");
	strncat(mutex_name,name,IP_MAX_MEM_NAME_LENGTH-1);
	mutex_name[IP_MAX_MEM_NAME_LENGTH+5-1]='\0';

    sm->ghMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        mutex_name);             // unnamed mutex

    if (sm->ghMutex == NULL)
    {
        printf("CreateMutex error: %d\n", GetLastError());
        printf("blah\n");
    }


	return sm;
}

/*
 * Destroy Shared memory and deallocate memory
 *
 * WARNING. This may destroy all data in shared memory.
 */
int destroySharedMemoryObj(SharedMemory_handle sm){
	if (sm!=NULL){
		sm->name[0]='\0';
		/*Close out the shared memory file */
		if(sm->pBuf!=NULL) UnmapViewOfFile((PVOID) sm->pBuf);
		if(sm->hMapFile!=NULL) CloseHandle(sm->hMapFile);
		if(sm->ghMutex!=NULL) CloseHandle(sm->ghMutex);
		free(sm);
		sm=NULL;
	}
	return IP_SUCCESS;
}



/*************
 *
 * Create and Destroy Shared Data
 *
 */

/*
 * Create Shared Data chunk.
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
/*
 * destroy Shared Data chunk
 */
int destroySharedData(SharedData_t* sd){
	if (sd!=NULL){
		/** Blank out the fields with zeros **/
		sd->usedFields=0;
		int k=0;
		for (k = 0; k < sd->maxNumFields; ++k) {
			/* clear field */
			zeroField( &(sd->fields[k]) ) ;
		}


	}
	free(sd);
	return IP_SUCCESS;
}


/*
 * Do a simple sanity check on the Shared Data Struct
 */
int verifySharedDataStruct(SharedData_t* sd){
  if (sd->maxNumFields >0){
	  return IP_SUCCESS;
  } else {
	  return IP_ERROR;
  }
}

/********************
 *
 * Get Lock
 *
 */

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
int AcquireLock(SharedMemory_handle sm){
	/*Try to Attain Mutex Lock */

	// Request ownership of mutex.
	DWORD dwWaitResult;

    dwWaitResult = WaitForSingleObject(
    		sm->ghMutex,    // handle to mutex
    		sm->ReadTimeDelay);  // no time-out interval

		        switch (dwWaitResult)
		        {
		            // The thread got ownership of the mutex
		            case WAIT_OBJECT_0:
		            	return IP_SUCCESS;

		            // The thread got ownership of an abandoned mutex
		            case WAIT_ABANDONED:

		                return IP_BUSY;
		        }
}

/*
 * Tries to release the mutex. Returns IP_ERROR or IP_SUCCESS;
 */
int ReleaseLock(SharedMemory_handle sm){
	// Release ownership of the mutex object
	if (! ReleaseMutex(sm->ghMutex))
	{
		printf("ERROR: Unable to release mutex\n");
		return IP_ERROR;
	} else {
		return IP_SUCCESS;
	}

}


/*********************************************************************************/
/*********************************************************************************/
/* 			P U B L I C        F U N C T I O N S                                 */
/*********************************************************************************/
/*********************************************************************************/



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
	/* Create NULL local shared memory object **/
	SharedMemory_handle sm =NULL;

   HANDLE hMapFile;
   LPCTSTR pBuf;

   /** Create file mapping **/
   hMapFile = CreateFileMapping(
				 INVALID_HANDLE_VALUE,    // use paging file
				 NULL,                    // default security
				 PAGE_READWRITE,          // read/write access
				 0,                       // maximum object size (high-order DWORD)
				 IP_BUF_SIZE,                // maximum object size (low-order DWORD)
				 name);                 // name of mapping object
   if (hMapFile == NULL)
   {
	  _tprintf(TEXT("Could not create file mapping object (%d).\n"),
			 GetLastError());
	  return sm;
   }


   /* Create a buffer for the map opbject*/
   pBuf = (LPTSTR) MapViewOfFile(hMapFile,   // handle to map object
						FILE_MAP_ALL_ACCESS, // read/write permission
						0,
						0,
						IP_BUF_SIZE);

   if (pBuf == NULL)
   {
	  _tprintf(TEXT("Could not map view of file (%d).\n"),
			 GetLastError());

	   CloseHandle(hMapFile);

	  return sm;
   }


	/* Create Local Shared Memory Object to Store Information */
	sm=createSharedMemoryObj(name,hMapFile,pBuf);

	/* Create a Local Copy of the Shared Data Object (and mutex)*/
	struct SharedData_t* local_sd=createSharedData();

	/*Try to Attain Mutex Lock */

	    // Request ownership of mutex.
		DWORD dwWaitResult;

	        dwWaitResult = WaitForSingleObject(
	            sm->ghMutex,    // handle to mutex
	            INFINITE);  // no time-out interval

	        switch (dwWaitResult)
	        {
	            // The thread got ownership of the mutex
	            case WAIT_OBJECT_0:

					/* We've received lock.. perform an action **/
	            	/* Copy the local copy of the Shared Data Object into Shared Memory */
	                CopyMemory((PVOID)pBuf, &local_sd, sizeof(local_sd));

					// Release ownership of the mutex object
					if (! ReleaseMutex(sm->ghMutex))
					{
						printf("ERROR: Unable to release mutex\n");
					}
	                break;

	            // The thread got ownership of an abandoned mutex
	            case WAIT_ABANDONED:
	                printf("The mutex appears to be abandoned! Sad\n");
	                /*Destroy the local copy of the Shared Data object */
	                destroySharedData(local_sd);
	                return NULL;
	        }


    /* Update the Shared MEmory Obj to reflect that the local data is now in shared MEmory */
    sm->sd=(SharedData_t*) pBuf;

    /*Destroy the local copy of the Shared Data object */
    destroySharedData(local_sd);

	return sm;
}





/*
 * Start the shared memory client and create a shared memory object.
 * This is to be run on the client process.
 */
SharedMemory_handle ip_CreateSharedMemoryClient(char* name){
	/* Create NULL local shared memory object **/
	SharedMemory_handle sm =NULL;

   HANDLE hMapFile;
   LPCTSTR pBuf;

   /** Create file mapping **/
   hMapFile = OpenFileMapping(
                   FILE_MAP_ALL_ACCESS,   // read/write access
                   FALSE,                 // do not inherit the name
                   name);               // name of mapping object

   if (hMapFile == NULL)
   {
	  _tprintf(TEXT("Could not create file mapping object (%d).\n"),
			 GetLastError());
	  return sm;
   }


   /* Create a buffer for the map opbject*/
   pBuf = (LPTSTR) MapViewOfFile(hMapFile,   // handle to map object
						FILE_MAP_ALL_ACCESS, // read/write permission
						0,
						0,
						IP_BUF_SIZE);

   if (pBuf == NULL)
   {
	  _tprintf(TEXT("Could not map view of file (%d).\n"),
			 GetLastError());

	   CloseHandle(hMapFile);

	  return sm;
   }





	/* Create Mutex */

	/*Attain Mutex Lock */


	/* Check to see that Shared Data Struct is Valid */
	if (verifySharedDataStruct((SharedData_t*) pBuf)== IP_SUCCESS){

		/* Create Local Shared Memory Object to Store Information */
		sm=createSharedMemoryObj(name,hMapFile,pBuf);
		/* Update the Shared MEmory Obj to reflect that shared data  is now in shared MEmory */
		sm->sd=(SharedData_t*) pBuf;

	} else{
		printf("Error. Shared Memory Map does not contain a valid shared data structure.\n");
	}

	/* if sm is null something went wrong */
	return sm;
}


/*
 *  Close the shared memory.
 *  Returns 0 if success (IP_SUCCESS).
 *  Returns -1 if error (IP_ERROR).
 */
int ip_CloseSharedMemory(SharedMemory_handle sm){

	return destroySharedMemoryObj(sm);

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

