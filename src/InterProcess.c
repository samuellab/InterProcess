/*
 ============================================================================
 Name        : InterProcess.c
 Author      : Andy
 Version     :
 Copyright   : GPL
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(void) {
	puts("!!!Hello World2!!!"); /* prints !!!Hello World!!! */

	#define BUF_SIZE 256
	TCHAR szName[]=TEXT("Global\\MyFileMappingObject");
	TCHAR szMsg[]=TEXT("Message from first process.");




	return EXIT_SUCCESS;
}
