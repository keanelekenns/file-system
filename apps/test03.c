#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include "../io/File.h"

//This test imitates the role a user application writing a large amount of data to a file.
int main(){
	printf("TEST03:\n\n");
	init_LLFS();//Don't need to do this if previous tests have initialized it, I just want to make sure I have a clean slate.
	printf("OPEN A FILE IN ROOT DIRECTORY (CREATE)\n\n");
	int fd = Open("/hello");
	printf("Filename: /hello\nFile descriptor (inode id): %d\n",fd);
	printf("Filesize Before: %d bytes\n\n",file_size(fd));
	printf("WRITE \'$\' SYMBOL TO FILE (1.5MB)\n\n");
	char* data = (char*)malloc(1500000);
	memset(data, '$', 1500000);
	write(fd, data, 1500000);
	printf("Filesize After: %d bytes\n\n",file_size(fd));
	printf("The wacky hexdump actually means it's working\n2929.6875 blocks should be used for data,\n11.40625 blocks for single indirection,\n1 double indirection block,\nand 1 file inode (total = 2944 blocks)\n\nTherefore the free block should display that the first 12 blocks are taken,\nbecause the first 10 are reserved + the root inode + our file's inode,\nand 2944 data blocks are reserved from the data block area\n(1 block for root directory and 2943 for our file's data blocks).\nThis can be verified by looking at the free block and calculating that there are\nexactly 2944 bits of zero value in the data block area.\n\n");
	printf("Notice that the root inode now has a filesize of 0x40 (two entries).\nThe inode for the \"hello\" file has it's filesize in the first int as well (which is much larger)\nand it has direct block adresses as well as indirect (single & double) block addresses.\n The non dollar sign sections of memory (after the inodes) are the single and double indirect data blocks.\nFeel free to inspect the addresses (although you probably don't want to at this point), they all line up quite beautifully.\n You can also count that there are 12 single indirect blocks and one double indirect block.\n\n");
	free(data);
	return 0;
}