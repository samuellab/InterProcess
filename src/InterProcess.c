/*
 ============================================================================
 Name        : InterProcess.c
 Author      : Andy
 Version     :
 Copyright   : GPL
 Description : Hello World in C, Ansi-style
 ============================================================================


 Note, most of this example is taken from:
 http://msdn.microsoft.com/en-us/library/aa366551%28v=vs.85%29.aspx

 "Creating Named Shared Memory" from the Microsoft Developers Network"

 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <tchar.h>




	#define BUF_SIZE 256
	TCHAR szName[]=TEXT("Global\\MyFileMappingObject");
	TCHAR szMsg[]=TEXT("Message from first process.");


int main(void) {
	puts("!!!Hello World2!!!"); /* prints !!!Hello World!!! */

	   HANDLE hMapFile;
	   LPCTSTR pBuf;


	   hMapFile = CreateFileMapping(
	                 INVALID_HANDLE_VALUE,    // use paging file
	                 NULL,                    // default security
	                 PAGE_READWRITE,          // read/write access
	                 0,                       // maximum object size (high-order DWORD)
	                 BUF_SIZE,                // maximum object size (low-order DWORD)
	                 szName);                 // name of mapping object

	   if (hMapFile == NULL)
	   {
	      _tprintf(TEXT("Could not create file mapping object (%d).\n"),
	             GetLastError());
	      return 1;
	   }



	   /* Create a buffer for the map opbject*/
	   pBuf = (LPTSTR) MapViewOfFile(hMapFile,   // handle to map object
	                        FILE_MAP_ALL_ACCESS, // read/write permission
	                        0,
	                        0,
	                        BUF_SIZE);

	   if (pBuf == NULL)
	   {
	      _tprintf(TEXT("Could not map view of file (%d).\n"),
	             GetLastError());

	       CloseHandle(hMapFile);

	      return 1;
	   }



	   /* Copy into the shared memory */
	   CopyMemory((PVOID)pBuf, szMsg, (_tcslen(szMsg) * sizeof(TCHAR)));


	   /* Wait for user input **/
	   _tprintf(TEXT("Press any key to end.\n"));
	    _getch();

	   /* Unmap */
	   UnmapViewOfFile((PVOID) pBuf);

	   /* Close the map */
	   CloseHandle(hMapFile);


	return EXIT_SUCCESS;
}
