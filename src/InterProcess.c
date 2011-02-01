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

/** Default refactory period (time delay) for client reads in ms**/
#define IP_DEFAULT_REFRACTORY_PERIOD 5

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
	struct field_t fields[(IP_BUF_SIZE / sizeof(struct field_t)) - 1];
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
SharedMemory_handle createSharedMemoryObj(char* name, HANDLE hMapFile,
		LPCTSTR pBuf);

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
 * Create field
 * (uses malloc() to allocate memory for field..
 * Don't forget to call destroyField()
 *
 * Returns NULL if there is an error.
 */
struct field_t* createField(char* name);

int destroyField(struct field_t** f);

/*
 * Adds a field to a shared data struct.
 * If a field of the same name already exists, that field is overwritten
 * The data object is full, the function returns IP_NO_MORE_ROOM, -3
 * Otherwise the function returns IP_ERROR -1, or IP_SUCCES 0.
 */
int addFieldToSharedData(struct field_t* f, struct SharedData_t* sd);

/*
 * Deletes a field to from a shared data struct given the field name.
 * Internally, this function moves the last field into the place of the
 * deleted field so that all used fields are contiguous.
 *
 * If the field does not exist, returns IP_ERROR -1
 * Otherwise, IP_SUCCES 0.
 */
int deleteFieldFromSharedData(char* name, struct SharedData_t* sd);

/*
 * Write data to a field
 */
int writeField(struct field_t* f, void *data, int dataSize);

/*
 * Copy a field
 * IP_SUCCESS
 * IP_ERROR
 *
 */
int copyField(struct field_t* dest, struct field_t* src);

/*********************************************************************************/
/*********************************************************************************/
/* 			P R I V A T E      F U N C T I O N S                                 */
/*********************************************************************************/
/*********************************************************************************/

/*
 * Zero a field
 */
int zeroField(struct field_t* field) {
	memset(field->name, '\0', sizeof(char) * IP_FIELD_NAME_SIZE );
	memset(field->data, '\0', sizeof(char) * IP_FIELD_DATA_CONTAINER_SIZE );
	return IP_SUCCESS;
}

/*
 * Create field
 * (uses malloc() to allocate memory for field..
 * Don't forget to call destroyField()
 *
 * Returns NULL if there is an error.
 */
struct field_t* createField(char* name) {
	if (strlen(name) > IP_FIELD_NAME_SIZE) {
		printf("Field name is too long in createField().\n");
		return NULL;
	}

	struct field_t* f = (struct field_t*) malloc(sizeof(struct field_t));
	strncpy(f->name, name, IP_FIELD_NAME_SIZE - 1);
	f->name[IP_FIELD_NAME_SIZE - 1] = '\0';
	return f;
}

int destroyField(struct field_t** f) {
	if (*f == NULL) {
		return IP_SUCCESS;
	}
	zeroField(*f);
	free(*f);
	*f = NULL;
	return IP_SUCCESS;
}

/*
 * Copy a field
 * IP_SUCCESS
 * IP_ERROR
 *
 */
int copyField(struct field_t* dest, struct field_t* src) {
	if (dest == NULL || src == NULL)
		return IP_ERROR;
	if (strlen(src->name) > IP_FIELD_NAME_SIZE) {
		printf("Src field name is too long in copyField().\n");
		return IP_ERROR;
	}

	zeroField(dest);

	strncpy(dest->name, src->name, IP_FIELD_NAME_SIZE - 1);
	dest->name[IP_FIELD_NAME_SIZE - 1] = '\0';
	memcpy(dest->data, src->data, src->size);
	dest->size = src->size;
	return IP_SUCCESS;
}

/*
 * Write data to a field
 */
int writeField(struct field_t* f, void *data, int dataSize) {
	if (f == NULL)
		return IP_DOES_NOT_EXIST;

	/** To be valid the field must have a legitimate name **/
	if (f->name == NULL) {
		printf("ERROR: name is null in writeField()\n");
		return IP_ERROR;
	}
	if (strlen(f->name) == 0) {
		printf("ERROR: name has two few characters\n");
		return IP_ERROR;
	}

	/** "delete" the current data in the field **/
	f->size = 0;

	/** Copy in the new data **/
	memcpy(f->data, data, dataSize);
	f->size = dataSize;
	return IP_SUCCESS;
}

/*
 * Find field of a given name in a shared Data Struct
 */
int findField(struct field_t** f, struct SharedData_t* sd, char* name) {
	if (name == NULL)
		return IP_ERROR;
	if (strlen(name) == 0)
		return IP_ERROR;
	if (verifySharedDataStruct(sd) == IP_ERROR)
		return IP_ERROR;

	int k = 0;
	for (k = 0; k < sd->usedFields; ++k) {
		if (strncmp(name, sd->fields[k].name, IP_FIELD_NAME_SIZE - 1) == 0) {
			/** Found a match! **/
			*f = &(sd->fields[k]);
			return IP_SUCCESS;
		}

	}
	return IP_DOES_NOT_EXIST;
}

/*
 * Adds a field to a shared data struct.
 * If a field of the same name already exists, that field is overwritten
 * The data object is full, the function returns IP_NO_MORE_ROOM, -3
 * Otherwise the function returns IP_ERROR -1, or IP_SUCCES 0.
 */
int addFieldToSharedData(struct field_t* f, struct SharedData_t* sd) {
	if (verifySharedDataStruct(sd) == IP_ERROR) {
		printf("ERROR: shared data struct is invalid in addFieldToSharedData()");
		return IP_ERROR;
	}
	if (f->name == NULL) {
		printf("ERROR: name is null in addFieldToSharedData()");
		return IP_ERROR;
	}

	struct field_t * dest_f = NULL;
	if (findField(&dest_f, sd, f->name) == IP_SUCCESS) {
		/** a field with that name already exists, so let's replace it **/
		copyField(dest_f, f);
	} else {
		/** A field of that name doesn't already exist.**/
		/** Let's see if we have room **/
		if (sd->usedFields < sd->maxNumFields) {
			/** there is room **/

			/** write to the n+1th field **/
			int ret = copyField(&(sd->fields[sd->usedFields]), f);
			printf(
					"In shared memory, *(sd->fields[sd->usedFields].data)is %d\n",
					(int) *(sd->fields[sd->usedFields].data));

			(sd->usedFields)++; /** important! increment # of fields used **/
			if (ret == IP_ERROR)
				printf("unable to copy field in addFieldToSharedData()\n");

			return ret;

		} else {
			/** no more room **/
			return IP_NO_MORE_ROOM;
		}

	}

}

/*
 * Deletes a field to from a shared data struct given the field name.
 * Internally, this function moves the last field into the place of the
 * deleted field so that all used fields are contiguous.
 *
 * If the field does not exist, returns IP_DOES_NOT_EXIST -2
 * Success IP_SUCCES 0.
 * Error IP_ERROR
 */
int deleteFieldFromSharedData(char* name, struct SharedData_t* sd) {
	if (strlen(name) == 0)
		return IP_ERROR;
	if (verifySharedDataStruct(sd) == IP_ERROR)
		return IP_ERROR;
	if (sd->usedFields == 0)
		return IP_DOES_NOT_EXIST; /** nothing to delete **/

	struct field_t* dest_f = NULL; /** field to delete **/

	if (findField(&dest_f, sd, name) == IP_SUCCESS) {
		/* delete the data in that field */
		zeroField(dest_f);
		if (sd->usedFields > 1) { /** If there is more than one field present **/
			/** copy the field in last place into the place where the deleted field was. **/
			writeField(dest_f, sd->fields[sd->usedFields - 1].data,
					sd->fields[sd->usedFields - 1].size);

			/** Clear the last field **/
			zeroField(&(sd->fields[sd->usedFields - 1]));
		}
		(sd->usedFields)--;

	} else {
		return IP_DOES_NOT_EXIST;
	}
}

/*
 * and when you write the delete function,
 * swap in the nth' field for the field you are deleting
 * (unless of course you happen to be deleting the n'th field)
 *
 */

/*
 * Create Shared Memory Object
 *
 */
SharedMemory_handle createSharedMemoryObj(char* name, HANDLE hMapFile,
		LPCTSTR pBuf) {
	/* Create Local Shared Memory Object to Store Information */
	SharedMemory_handle sm = (SharedMemory_handle) malloc(
			sizeof(struct SharedMemory_t));

	/* Initialize the Local Shared Memory Object */
	strncpy(sm->name, name, IP_MAX_MEM_NAME_LENGTH - 1);
	sm->name[IP_MAX_MEM_NAME_LENGTH - 1] = '\0';
	sm->BufferSize = IP_BUF_SIZE;
	sm->ReadTimeDelay = IP_DEFAULT_REFRACTORY_PERIOD;
	sm->pBuf = pBuf;
	sm->hMapFile = hMapFile;
	sm->sd = NULL;

	/** Create Mutex for Shared Memory **/
	// Create a mutex with no initial owner
	sm->ghMutex = NULL;
	sm->lockWaitTime = 4;

	/* give the mutex the same name as the memory object but with the prefix "mutex_" */
	char mutex_name[IP_MAX_MEM_NAME_LENGTH + 5];
	strcpy(mutex_name, "mutex_");
	strncat(mutex_name, name, IP_MAX_MEM_NAME_LENGTH - 1);
	mutex_name[IP_MAX_MEM_NAME_LENGTH + 5 - 1] = '\0';

	sm->ghMutex = CreateMutex(NULL, // default security attributes
			FALSE, // initially not owned
			mutex_name); //mutex

	if (sm->ghMutex == NULL) {
		printf("CreateMutex error: %d\n", GetLastError());
	}
	return sm;
}

/*
 * Destroy Shared memory and deallocate memory
 *
 * WARNING. This may destroy all data in shared memory.
 */
int destroySharedMemoryObj(SharedMemory_handle sm) {
	if (sm != NULL) {
		sm->name[0] = '\0';
		/*Close out the shared memory file */
		if (sm->pBuf != NULL)
			UnmapViewOfFile((PVOID) sm->pBuf);
		if (sm->hMapFile != NULL)
			CloseHandle(sm->hMapFile);
		if (sm->ghMutex != NULL)
			CloseHandle(sm->ghMutex);
		free(sm);
		sm = NULL;
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
struct SharedData_t* createSharedData() {
	struct SharedData_t* sdata = (struct SharedData_t*) malloc(
			sizeof(struct SharedData_t));
	if (sdata != NULL) {
		sdata->usedFields = 0;
		sdata->maxNumFields = (IP_BUF_SIZE / sizeof(struct field_t)) - 1;

		/** Blank out the data with zeros **/
		int k = 0;
		for (k = 0; k < sdata->maxNumFields; ++k) {
			/* clear field */
			zeroField(&(sdata->fields[k]));
		}

		/** Create the mutex **/

	}
	return sdata;

}
/*
 * destroy Shared Data chunk
 */
int destroySharedData(SharedData_t* sd) {
	if (sd != NULL) {
		/** Blank out the fields with zeros **/
		sd->usedFields = 0;
		int k = 0;
		for (k = 0; k < sd->maxNumFields; ++k) {
			/* clear field */
			zeroField(&(sd->fields[k]));
		}

	}
	free(sd);
	return IP_SUCCESS;
}

/*
 * Do a simple sanity check on the Shared Data Struct
 */
int verifySharedDataStruct(SharedData_t* sd) {
	if (sd->maxNumFields > 0) {
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
int AcquireLock(SharedMemory_handle sm) {
	/*Try to Attain Mutex Lock */
	if (sm->ghMutex == NULL) {
		printf("DUDE! sm->ghMutex is null!\n");

	}
	// Request ownership of mutex.
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(sm->ghMutex, // handle to mutex
			sm->lockWaitTime); // no time-out interval
	switch (dwWaitResult) {
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
int ReleaseLock(SharedMemory_handle sm) {
	// Release ownership of the mutex object
	if (!ReleaseMutex(sm->ghMutex)) {
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
SharedMemory_handle ip_CreateSharedMemoryHost(char* name) {
	/* Create NULL local shared memory object **/
	SharedMemory_handle sm = NULL;

	HANDLE hMapFile;
	LPCTSTR pBuf;

	/** Create file mapping **/
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, // use paging file
			NULL, // default security
			PAGE_READWRITE, // read/write access
			0, // maximum object size (high-order DWORD)
			IP_BUF_SIZE, // maximum object size (low-order DWORD)
			name); // name of mapping object
	if (hMapFile == NULL) {
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
				GetLastError());
		return sm;
	}

	/* Create a buffer for the map opbject*/
	pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
			FILE_MAP_ALL_ACCESS, // read/write permission
			0, 0, IP_BUF_SIZE);

	if (pBuf == NULL) {
		_tprintf(TEXT("Could not map view of file (%d).\n"), GetLastError());

		CloseHandle(hMapFile);

		return sm;
	}

	/* Create Local Shared Memory Object to Store Information */
	sm = createSharedMemoryObj(name, hMapFile, pBuf);

	/* Create a Local Copy of the Shared Data Object (and mutex)*/
	struct SharedData_t* local_sd = createSharedData();

	/*Try to Attain Mutex Lock */
	if (AcquireLock(sm) == IP_SUCCESS) {
		/* Copy the local copy of the Shared Data Object into Shared Memory */
		CopyMemory((PVOID) pBuf, &local_sd, sizeof(local_sd));
		// Release ownership of the mutex object
		ReleaseLock(sm);

		/* Update the Shared MEmory Obj to reflect that the local data is now in shared MEmory */
		sm->sd = (SharedData_t*) pBuf;

	} else {
		printf("The mutex appears to be busy!. Sad. \n");
		sm = NULL;
	}

	/*Destroy the local copy of the Shared Data object */
	destroySharedData(local_sd);
	return sm;
}

/*
 * Start the shared memory client and create a shared memory object.
 * This is to be run on the client process.
 */
SharedMemory_handle ip_CreateSharedMemoryClient(char* name) {
	/* Create NULL local shared memory object **/
	SharedMemory_handle sm = NULL;

	HANDLE hMapFile;
	LPCTSTR pBuf;

	/** Create file mapping **/
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, // read/write access
			FALSE, // do not inherit the name
			name); // name of mapping object

	if (hMapFile == NULL) {
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
				GetLastError());
		return sm;
	}

	/* Create a buffer for the map opbject*/
	pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
			FILE_MAP_ALL_ACCESS, // read/write permission
			0, 0, IP_BUF_SIZE);

	if (pBuf == NULL) {
		_tprintf(TEXT("Could not map view of file (%d).\n"), GetLastError());

		CloseHandle(hMapFile);

		return sm;
	}

	/* Create Local Shared Memory Object to Store Information */
	sm = createSharedMemoryObj(name, hMapFile, pBuf);
	/* Update the Shared MEmory Obj to reflect that shared data  is now in shared MEmory */
	sm->sd = (SharedData_t*) pBuf;




	 /*Attain Mutex Lock */
	if (AcquireLock(sm) == IP_SUCCESS) {

		/* Check to see that Shared Data Struct is Valid */
		if (verifySharedDataStruct(sm->sd) == IP_SUCCESS) {
			printf("SUCCESS: Shared data structure verified!\n");
			printf("sm->sd->usedFields=	 %d\n", sm->sd->usedFields);
		} else {
			printf("ERROR: The Shared Data Struct is not valid!\n");
			return NULL;
		}
		ReleaseLock(sm);
	} else {
		printf(
				"Shared memory busy.\nUnable to attain lock to verify the shared data structs.\n");
		destroySharedMemoryObj(sm);
		sm = NULL;
	}

	/* if sm is null something went wrong */
	return sm;
}

/*
 *  Close the shared memory.
 *  Returns 0 if success (IP_SUCCESS).
 *  Returns -1 if error (IP_ERROR).
 */
int ip_CloseSharedMemory(SharedMemory_handle sm) {

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
char* ip_GetSharedMemoryName(SharedMemory_handle sm) {
	return sm->name;

}

/*
 * Get the size of the shared memory.
 *
 */
int ip_GetSharedMemorySize(SharedMemory_handle sm) {
	return sm->BufferSize;

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
int ip_GetSharedMemoryStatus(SharedMemory_handle sm) {
	if (sm == NULL) {
		return IP_DOES_NOT_EXIST;
	}

	if (sm->sd == NULL) {
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	if (AcquireLock(sm) == IP_BUSY) {
		return IP_BUSY;

	} else {

		if (verifySharedDataStruct(sm->sd) == IP_ERROR) {
			ReleaseLock(sm);
			printf("Shared Data struct in shared memory is invalid.\n");
			return IP_ERROR;
		}

		ReleaseLock(sm);
		return IP_SUCCESS;

	}
}

int ip_SetSharedMemoryLockWaitTime(SharedMemory_handle sm, int time_ms) {
	if (sm != NULL) {
		sm->lockWaitTime = (DWORD) time_ms;
		return IP_SUCCESS;
	} else {
		return IP_ERROR;
	}
}

int ip_GetSharedMemoryLockWaitTime(SharedMemory_handle sm) {
	if (sm == NULL)
		return IP_ERROR;
	return (int) sm->lockWaitTime;
}


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
int ip_GetSharedMemoryReadRefractoryPeriodTimeDelay(SharedMemory_handle sm, int* time_ms){
	if (sm==NULL) return IP_ERROR;
	*time_ms = sm->ReadTimeDelay;
	return IP_SUCCESS;
}

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
int ip_SetSharedMemoryReadRefractoryPeriodTimeDelay(SharedMemory_handle sm, int time_ms){
	if (sm==NULL) return IP_ERROR;
	sm->ReadTimeDelay=time_ms;
	return IP_SUCCESS;
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
int ip_WriteValue(SharedMemory_handle sm, char* fieldName, void *data,
		int dataSize) {
	if (sm == NULL)
		return IP_DOES_NOT_EXIST;
	if (strlen(fieldName) < 1)
		return IP_ERROR;

	if (sm->sd == NULL) {
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	if (AcquireLock(sm) == IP_BUSY)
		return IP_BUSY;

	if (verifySharedDataStruct(sm->sd) == IP_ERROR) {
		ReleaseLock(sm);
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	/** Action Happens Here **/

	/** Create the Field **/
	struct field_t* f = createField(fieldName);
	int ret = writeField(f, data, dataSize);
	if (ret == IP_SUCCESS) {
		ret = addFieldToSharedData(f, sm->sd);
	}

	ReleaseLock(sm);
	destroyField(&f);

	return ret;

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
int ip_ReadValue(SharedMemory_handle sm, char* fieldName, void *data) {
	if (sm == NULL)
		return IP_DOES_NOT_EXIST;

	if (sm->sd == NULL) {
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	if (AcquireLock(sm) == IP_BUSY)
		return IP_BUSY;

	if (verifySharedDataStruct(sm->sd) == IP_ERROR) {
		ReleaseLock(sm);
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	/** Action Happens Here **/

	/** Read out the field **/
	struct field_t* f = NULL;
	int ret = findField(&f, sm->sd, fieldName);
	if (ret == IP_SUCCESS) {
		memcpy(data, f->data, f->size);
	}

	ReleaseLock(sm);

	/** Refractory Period **/
	Sleep(sm->ReadTimeDelay);// sleep the thread for a little bit so that other threads can capture lock

	return ret;

}

/*
 * Remove all fields in shared memory
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearAllFields(SharedMemory_handle sm) {

	if (sm == NULL)
		return IP_DOES_NOT_EXIST;

	if (sm->sd == NULL) {
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	if (AcquireLock(sm) == IP_BUSY)
		return IP_BUSY;

	if (verifySharedDataStruct(sm->sd) == IP_ERROR) {
		ReleaseLock(sm);
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	int k = 0;
	for (k = 0; k < sm->sd->usedFields; ++k) {
		zeroField(&(sm->sd->fields[k]));

	}

	ReleaseLock(sm);
	return IP_SUCCESS;

}

/*
 *  Clear a single field in shared memory
 * Return Values:
 *  IP_SUCCESS 0
 *  IP_ERROR -1  if SharedMemory doesn't exist
 *  IP_BUSY 1
 *
 */
int ip_ClearField(SharedMemory_handle sm, char* fieldName) {
	if (sm == NULL)
		return IP_DOES_NOT_EXIST;

	if (sm->sd == NULL) {
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	if (AcquireLock(sm) == IP_BUSY)
		return IP_BUSY;

	if (verifySharedDataStruct(sm->sd) == IP_ERROR) {
		ReleaseLock(sm);
		printf("Shared Data struct in shared memory is invalid.\n");
		return IP_ERROR;
	}

	struct field_t* f = NULL;
	int ret = findField(&f, sm->sd, fieldName);
	if (ret == IP_SUCCESS) {
		zeroField(f);
	}
	ReleaseLock(sm);
	return ret;

}

