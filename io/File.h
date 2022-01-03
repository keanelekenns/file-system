#ifndef __LLFS__
#define __LLFS__

#define NUM_BLOCKS 4096
#define BLOCK_SIZE 512
#define NUM_INODES 128
#define INODE_SIZE 32
#define ENTRY_SIZE 32

int file_size(int fd);

int Open(char* pathname);

int Mkdir(char* pathname);

int Rmdir(char* pathname);

int Unlink(char* pathname);

int read(int fd, void* buffer, int length);

int write(int fd, void* buffer, int length);

void fsck();

void init_LLFS();

#endif