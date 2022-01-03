#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application creating subdirectories
int main(){
	printf("TEST05:\n\n");
	init_LLFS();//fresh start
	printf("MAKE DIRECTORY \"/a3\"\n\n");
	int fd = Mkdir("/a3");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/apps\"\n\n");
	fd = Mkdir("/a3/apps");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/io\"\n\n");
	fd = Mkdir("/a3/io");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/disk\"\n\n");
	fd = Mkdir("/a3/disk");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/disk/level3\"\n\n");
	fd = Mkdir("/a3/disk/level3");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/disk/level3/level4\"\n\n");
	fd = Mkdir("/a3/disk/level3/level4");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	
	printf("MAKE DIRECTORY \"/a3/disk/level3/level4/level5\"\n\n");
	fd = Mkdir("/a3/disk/level3/level4/level5");
	if(fd == 0){
		printf("FAILED TO OPEN DIRECTORY\n");
		return 1;
	}
	return 0;
}