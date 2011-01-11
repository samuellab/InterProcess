/*
 * client.c
 *
 *  Created on: Jan 7, 2011
 *      Author: andy
 */

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUF_SIZE 256
TCHAR szName[]=TEXT("Global\\MyFileMappingObject");

int _tmain()
{
   HANDLE hMapFile;
   LPCTSTR pBuf;

   hMapFile = OpenFileMapping(
                   FILE_MAP_ALL_ACCESS,   // read/write access
                   FALSE,                 // do not inherit the name
                   szName);               // name of mapping object

   /** Check to see if hMapFile was created correctly **/
   if (hMapFile == NULL)
   {
      _tprintf(TEXT("Could not open file mapping object (%d).\n"),
             GetLastError());
      return 1;
   }


   pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
               FILE_MAP_ALL_ACCESS,  // read/write permission
               0,
               0,
               BUF_SIZE);


   /** Check to see that the buffer exists **/
   if (pBuf == NULL)
   {
      _tprintf(TEXT("Could not map view of file (%d).\n"),
             GetLastError());

      CloseHandle(hMapFile);

      return 1;
   }
   int *intptr = (int *) pBuf;

   _tprintf("The other process says:\n");
   printf("%d and %d\n",intptr[0], intptr[1]);

   UnmapViewOfFile((PVOID) pBuf);

   CloseHandle(hMapFile);

   return 0;
}

