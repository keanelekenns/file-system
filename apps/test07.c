#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application deleting files and directories.
int main(){
	printf("TEST07:\n\n");
	printf("DO NOT INIT LLFS (USING PREVIOUS TEST VALUES)\n\n");
	printf("DELETING FILE \"/a3/report.txt\"\n\n");
	if(Unlink("/a3/report.txt")){
		printf("FAILED TO DELETE FILE\n\n");
	}
	printf("DELETING DIRECTORY \"/a3/io\"\n\n");
	if(Rmdir("/a3/io")){
		printf("FAILED TO DELETE DIRECTORY\n\n");
	}
	
	printf("ATTEMPTING TO DELETE DIRECTORY \"/a3/disk\" (This should fail because it is not empty)\n\n");
	if(Rmdir("/a3/disk")){
		printf("FAILED TO DELETE DIRECTORY\n\n");
	}
	
	return 0;
}