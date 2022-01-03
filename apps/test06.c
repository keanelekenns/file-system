#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application writing/reading files in subdirectories
int main(){
	printf("TEST06:\n\n");
	printf("DO NOT INIT LLFS (USING PREVIOUS TEST VALUES)\n\n");
	printf("CREATING FILE\n\n");
	int fd = Open("/a3/report.txt");
	if(fd == 0){
		printf("FAILED TO OPEN FILE. Make sure test05 is run immediately before this\n");
		return 1;
	}
	printf("Filename: /a3/report.txt\nFile descriptor (inode id): %d\n\n",fd);
	char* test_string = "Hello, I hope you are having an awesome day, then again, it probably isn't super fun to mark these, but you'll get through it! Trust me, I'm tired of writing it!\n";
	
	printf("WRITING STRING TO FILE\n\n");
	if(write(fd, test_string, 163)){
		printf("FAILED TO WRITE FILE\n");
		return 1;
	}
	
	printf("READING STRING FROM FILE (only first 44 bytes)\n\n");
	char* buffer = (char*)malloc(45);
	if(read(fd, buffer, 44)){
		printf("FAILED TO READ FILE\n");
		return 1;
	}
	buffer[44] = 0;
	printf("STRING CONTENTS: %s\n\n", buffer);
	free(buffer);
	return 0;
}