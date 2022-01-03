#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application attempting to read more bytes than a file contains.
int main(){
	printf("TEST04:\n\n");
	printf("DO NOT INIT LLFS (USING PREVIOUS TEST VALUES)\n\n");
	printf("OPEN FILE \"/hello\"\n\n");
	int fd = Open("/hello");
	if(fd == 0){
		printf("FAILED TO OPEN \"/hello\". Make sure test03 is run immediately before this\n");
		return 1;
	}
	printf("READ FILE (1.5MB)\n\n");
	printf("ATTEMPTING TO READ 2MB FROM FILE (read should stop at filesize)\n\n");
	char* data = (char*)malloc(2000000);
	if(read(fd, data, 2000000)){
		printf("FAILED TO READ \"/hello\". Make sure test03 is run immediately before this\n");
		return 1;
	}
	printf("DATA BUFFER AT INDEX 0: %c\nDATA BUFFER AT INDEX filesize - 1: %c\nDATA BUFFER AT INDEX filesize: %c\n\n", data[0], data[file_size(fd)-1], data[file_size(fd)]);
	
	free(data);
	printf("There is no hexdump for this test (it would be the same as the previous test).\n\n");
	return 0;
}