#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include "disk_controller.h"

//All of these functions (apart from block_is_empty) return 0 upon success
//and -1 upon failure

int write_block(FILE* vdisk, int block_number, void* buffer){
	if(block_number < 0 || block_number > NUM_BLOCKS -1){
		fprintf(stderr, "\nERROR:\nFailed to write a block because index %d is out of range\n\
		Please use an index from 0 to %d\n",block_number,NUM_BLOCKS-1);
		return -1;
	}
	if(fseek(vdisk, block_number*BLOCK_SIZE, SEEK_SET)){
		fprintf(stderr, "\nERROR:\nUnable to find block\n");
		return -1;
	}
	if(fwrite(buffer, BLOCK_SIZE, 1, vdisk) == 0){
		fprintf(stderr, "\nERROR:\nUnable to write block\n");
		return -1;
	}
	return 0;
}

int read_block(FILE* vdisk, int block_number, void* buffer){
	if(block_number < 0 || block_number > NUM_BLOCKS -1){
		fprintf(stderr, "\nERROR:\nFailed to read a block because index %d is out of range\n\
		Please use an index from 0 to %d\n",block_number,NUM_BLOCKS-1);
		return -1;
	}
	if(fseek(vdisk, block_number*BLOCK_SIZE, SEEK_SET)){
		fprintf(stderr, "\nERROR:\nUnable to find block\n");
		return -1;
	}
	if(fread(buffer, BLOCK_SIZE, 1, vdisk) == 0){
		fprintf(stderr, "\nERROR:\nUnable to read block\n");
		return -1;
	}
	return 0;
}

int init_vdisk(){
	FILE* fp = fopen("../disk/vdisk", "wb");//If vdisk exists, this will wipe it clean. If not, it creates one.
    if(fp == NULL || fseek(fp, NUM_BLOCKS*BLOCK_SIZE - 1, SEEK_SET)){
		fprintf(stderr, "\nERROR:\nUnable to initialize vdisk\n");
		return -1;
	}
    fputc('\0', fp);
    return fclose(fp);
}

//return 0 if block has a nonzero value in it, return 1 if it is all zeros
//return -1 upon failure
int block_is_empty(FILE* vdisk, int block_number){
	if(block_number < 0 || block_number > NUM_BLOCKS -1){
		fprintf(stderr, "\nERROR:\nFailed to read a block because index %d is out of range\n\
		Please use an index from 0 to %d\n",block_number,NUM_BLOCKS-1);
		return -1;
	}
	if(fseek(vdisk, block_number*BLOCK_SIZE, SEEK_SET)){
		fprintf(stderr, "\nERROR:\nUnable to find block\n");
		return -1;
	}
	
    for(int i = 0; i < BLOCK_SIZE; i++) {
        if(fgetc(vdisk) != 0) return 0;
    }
    return 1;
}