#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../disk/disk_controller.h"

//This test imitates the role of the file system, not a user level application (it is using the disk controller)
int main(){
	
	printf("TEST01:\n\n");
	printf("INIT VDISK\n\n");
	if(init_vdisk()){
		//This would be what test02.c showed
		//(it's pretty straightforward and I had to do it here anyways, so I merged them)
		//it simply creates a file named vdisk and makes it NUM_BLOCKS*BLOCK_SIZE bytes big
		//it also wipes it clean if it exists (see init_vdisk in disk_controller.c)
		return -1;
	}
	
	FILE* fp = fopen("../disk/vdisk", "rb+");//MOUNTING DISK
	if(fp == NULL){
		fprintf(stderr, "\nERROR:\nvdisk does not exist\n");
		return -1;
	}
	char* write_test = (char*)calloc(BLOCK_SIZE, 1);//It is the responsibility of the file system to write a 512 byte block
	
	char* hello = "hello world!\n";
	
	for(int i =0; i < strlen(hello); i++){
		write_test[i] = hello[i];
	}
	printf("WRITING STRING TO BLOCK 3095:\n\n%s\n", write_test);
	if(write_block(fp, 3095, write_test)){//write to block 3095
		//Upon failure of a write/read to a block, I decided
		//to make functions exit with a failure value
		//The later implications of this was that
		//each write/read had a large chunk of code devoted
		//to freeing up all of the malloc'd memory
		free(write_test);
		fclose(fp);
		return -1;
	}
	free(write_test);
	
	
	char* read_test = (char*)malloc(BLOCK_SIZE);
	
	printf("READING STRING FROM BLOCK 3095\n\n");
	if(read_block(fp, 3095, read_test)){// read from block 3095
		free(read_test);
		fclose(fp);
		return -1;
	}
	printf("CONTENTS OF STRING READ FROM BLOCK 3095:\n\n%s\n", read_test);//print out what we read (stops at null byte)
	printf("WRITING READ STRING TO BLOCK 4095\n\n");
	if(write_block(fp, 4095, read_test)){//write what we read to block 4095
		free(read_test);
		fclose(fp);
		return -1;
	}
	free(read_test);
	
	printf("TESTING block_is_empty FUNCTION:\n\n");
	printf("BLOCK 4095 (expect 0 because it is not empty): %d\nBLOCK 4005 (expect 1): %d\n\n", block_is_empty(fp, 4095), block_is_empty(fp, 4005));
	
	fclose(fp);
	printf("At this point, the hexdump of vdisk should display two identical blocks with hello world!\\n in them.\nNote that 3095*BLOCK_SIZE == 0x182e00 and 4095*BLOCK_SIZE == 0x1ffe00 as you will see in the hexdump.\n\n");
	return 0;
}