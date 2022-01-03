#ifndef __DISK_CONTROLLER__
#define __DISK_CONTROLLER__

#define NUM_BLOCKS 4096
#define BLOCK_SIZE 512

int write_block(FILE* vdisk, int block_number, void* buffer);

int read_block(FILE* vdisk, int block_number, void* buffer);

int init_vdisk();

int block_is_empty(FILE* vdisk, int block_number);

#endif