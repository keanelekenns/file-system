#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application initializing the file system.
int main(){
	printf("TEST02:\n\n");
	printf("FORMAT VDISK WITH INIT LLFS\n\n");
	init_LLFS();
	printf("First section of memory is the super block.\nThis is formatted according to the assignment writeup\n(i.e. Three ints: magic number, number of blocks on disk, number of inodes on disk)\n\n");
	printf("Second section of memory is the free block.\nThis is formatted according to the assignment writeup\n(i.e. A set bit indicates that the block is free whereas a zero bit\nindicates it is reserved, notice that there is a d in row 2 indicating that the root's data block is reserved)\n\n");
	printf("Address 0x1400 (Block index 10):\nThis is an inode for the root directory.\nThe first 4 bytes indicate the file size (which\nin this case is 0x20 = 32 = ENTRY_SIZE because it has one entry).\nThe next int (4 bytes) is the flag, which is set to 1, which identifies a directory.\nEvery two bytes after this is a block number.\nIn this case the first block number is 0x8a, which, when multiplied by BLOCKSIZE, gives the next memory address displayed.\n\n");
	printf("Address 0x11400 (Block index 138):\nThis is a data block for the root directory.\nThis is formatted according to the assignment writeup.\nThat is, the first byte indicates the inode id (in this case 1, for the root)\nand the next 31 bytes are the file name of the entry (which is simply \'.\' in this case).\n\n");
	return 0;
}