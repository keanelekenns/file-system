/*
Spring 2019 CSC 360 Assignment 3

Filename: File.c
Author: Keanelek Enns V00875807
*/
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<assert.h>
#include<stdbool.h>
#include "../disk/disk_controller.h"
#include "File.h"

/*
//Return 1 if bit is set, else return 0.
//It is the responsibility of the caller
//to ensure the value specified for bit
//falls within the buffer. Indexing starts at 0
*/
int check_bit(void* buffer, int bit){
	int byte = bit/8;
	bit = bit%8;
	uint8_t bit_mask = 0x80 >> bit;
	if(((uint8_t*)buffer)[byte] & bit_mask){
		return 1;
	}
	return 0;
}

/*
//Sets bit specified to 1 in buffer.
//It is the responsibility of the caller
//to ensure the value specified for bit
//falls within the buffer. Buffer must be mutable.
//Indexing starts at 0
*/
void set_bit(void* buffer, int bit){
	int byte = bit/8;
	bit = bit%8;
	uint8_t bit_mask = 0x80 >> bit;
	((uint8_t*)buffer)[byte] = ((uint8_t*)buffer)[byte] | bit_mask;
	return;
}

/*
//Sets bit specified to 0 in buffer.
//It is the responsibility of the caller
//to ensure the value specified for bit
//falls within the buffer. Buffer must be mutable.
//Indexing starts at 0
*/
void unset_bit(void* buffer, int bit){
	int byte = bit/8;
	bit = bit%8;
	uint8_t bit_mask = 0x80 >> bit;
	bit_mask = 0xFF - bit_mask;
	((uint8_t*)buffer)[byte] = ((uint8_t*)buffer)[byte] & bit_mask;
	return;
}

/*
//returns an index to the first available data block
// in the copy of free_block given or -1 if it fails
*/
int available_data_block(FILE* vdisk, void* free_block){
	
	for(int i = NUM_INODES + 10; i < NUM_BLOCKS; i++){
		if(check_bit(free_block, i)){
			return i;
		}
	}
	return -1;
}

/*
//returns an inode id for the first available inode block
//in the copy of free_block given or -1 if it fails.
*/
int available_inode_id(FILE* vdisk, void* free_block){

	for(int i = 10; i < NUM_INODES + 10; i++){
		if(check_bit(free_block, i)){
			return i-9;
		}
	}
	return -1;
}

/*
// Return the file size in bytes of the file 
// indicated by the given inode id (fd).
// Return -1 upon failure.
*/
int file_size(int fd){
	
	if(fd < 1 || fd > NUM_INODES){
		return -1;//invalid inode id
	}
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -1;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, fd + 9, inode)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	int filesize = ((int*)inode)[0];
	free(inode);
	fclose(vdisk);
	return filesize;
	
}

/*
//Create a file in the directory indicated by the given inode id
//This function returns the new file's inode id upon success and
//zero upon failure. flags can be set for the new file.
// A flag of 0 creates a normal file, a flag of 1 creates a directory.
*/
int create_at(int dir, char* filename, int flags){
	if(dir < 1 || dir > NUM_INODES){
		return 0;//invalid inode id
	}
	if(strlen(filename) > 30){
		return 0;//filename is too long
	}
	
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return 0;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, dir + 9, inode)){
		free(inode);
		fclose(vdisk);
		return 0;
	}
	if(((int*)inode)[1] != 1){//dir is not a directory's inode id
		free(inode);
		fclose(vdisk);
		return 0;
	}
	
	int filesize = ((int*)inode)[0];
	
	int file_block = filesize/BLOCK_SIZE;
	int file_entry = filesize%BLOCK_SIZE;
	
	//A directory's filesize is always increased/decreased by ENTRY_SIZE
	filesize += ENTRY_SIZE;
	((int*)inode)[0] = filesize;
	
	//NUM_INODES must not be greater than 192, and for this system NUM_INODES = 128
	assert(NUM_INODES <= 192);//guarantees file_block to be < 12 for indexing later
	
	if(filesize/ENTRY_SIZE >= NUM_INODES){
		free(inode);
		fclose(vdisk);
		return 0; //Already has every inode in the system
	}
	
	int new_inode_id;
	int new_block;
	
	char* free_block = (char*)malloc(BLOCK_SIZE);
	if(read_block(vdisk, 1, free_block)){
		free(free_block);
		free(inode);
		fclose(vdisk);
		return 0;
	}
	
	if(file_entry==0){//we must write to a new data block for directory
		
		new_block = available_data_block(vdisk, free_block);
	
		if(new_block == -1){
			free(free_block);
			free(inode);
			fclose(vdisk);
			return 0;
		}
	
		unset_bit(free_block, new_block);//reserve this data block
		((uint16_t*)inode)[file_block + 4] = new_block;
	}
	
	char* data_block = (char*)malloc(BLOCK_SIZE);		
	if(read_block(vdisk, ((uint16_t*)inode)[file_block + 4], data_block)){
		free(data_block);
		free(free_block);
		free(inode);
		fclose(vdisk);
		return 0;
	}
		
	new_inode_id = available_inode_id(vdisk, free_block);
	
	if(new_inode_id == -1){
		free(data_block);
		free(free_block);
		free(inode);
		fclose(vdisk);
		return 0;
	}
	
	unset_bit(free_block, new_inode_id + 9);//reserve this inode block
	//don't need to modify the new inode if it's just a file
	
	//do, however, need to modify it if it's a directory
	if(flags == 1){
		
		int data_block_index = available_data_block(vdisk, free_block);
		if(data_block_index == -1){
			free(data_block);
			free(free_block);
			free(inode);
			fclose(vdisk);
			return 0;
		}
		
		uint8_t* dir_data_block = (uint8_t*)calloc(BLOCK_SIZE,1);
		dir_data_block[0] = new_inode_id;//first entry of new directory
		dir_data_block[1] = '.';
		dir_data_block[ENTRY_SIZE] = dir;
		strncpy((char*)dir_data_block + ENTRY_SIZE + 1 , "..", 3);
		
		void* new_dir_inode = calloc(BLOCK_SIZE,1);
		((int*)new_dir_inode)[0] = ENTRY_SIZE*2;
		((int*)new_dir_inode)[1] = 1; //this file is a directory
		((uint16_t*)new_dir_inode)[4] = data_block_index; //block number for first data block
		
		unset_bit(free_block, data_block_index);
		
		if(write_block(vdisk, data_block_index, dir_data_block)){
			free(new_dir_inode);
			free(dir_data_block);
			free(data_block);
			free(free_block);
			free(inode);
			fclose(vdisk);
			return 0;
		}
		free(dir_data_block);
		
		if(write_block(vdisk, new_inode_id + 9, new_dir_inode)){
			free(new_dir_inode);
			free(data_block);
			free(free_block);
			free(inode);
			fclose(vdisk);
			return 0;
		}
		free(new_dir_inode);
	}
	
	data_block[file_entry] = new_inode_id;//insert the file inode into the above directory
	strncpy(data_block + file_entry + 1, filename, 31);
	
	
	
	if(write_block(vdisk, 1, free_block)){//reserved an inode block and possibly reserved a data block
		free(free_block);
		free(inode);
		free(data_block);
		fclose(vdisk);
		return 0;
	}
	free(free_block);
	
	//CRASH COULD OCCUR HERE
	
	if(write_block(vdisk, ((uint16_t*)inode)[file_block + 4], data_block)){//added a directory entry
		free(inode);
		free(data_block);
		fclose(vdisk);
		return 0;
	}
	free(data_block);
	
	//CRASH COULD OCCUR HERE
	
	if(write_block(vdisk, dir + 9, inode)){//changed filesize and possibly added a data block address
		free(inode);
		fclose(vdisk);
		return 0;
	}
	free(inode);
	fclose(vdisk);
	return new_inode_id;
}

/*
//search for the filename from the given directory's inode id
//if the file is found, return its inode id number.
//If the dir is not for a directory,
//then the function returns 0.
//If the file cannot be found in the directory,
//it will return -1.
//If some other error occurs, it will return -2
*/
int search_for_filename(int dir, char* filename){
	if(dir < 1 || dir > NUM_INODES){
		return -2;//invalid inode id
	}
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -2;
	}
	
	void* inode = (void*)malloc(BLOCK_SIZE);
	if(read_block(vdisk, dir + 9, (char*)inode)){
		free(inode);
		fclose(vdisk);
		return -2;
	}
	if(((int*)inode)[1] != 1){//this file is not a directory and we need not search it
		free(inode);
		fclose(vdisk);
		return 0;
	}
	
	int filesize = ((int*)inode)[0];
	
	char* data_block = (char*)malloc(BLOCK_SIZE);
	for(int i = 4; i < INODE_SIZE/sizeof(uint16_t); i++){//iterate through direct blocks
		if(read_block(vdisk, ((uint16_t*)inode)[i], data_block)){
			free(inode);
			free(data_block);
			fclose(vdisk);
			return -2;
		}
		for(int j = 1; j < BLOCK_SIZE; j+=ENTRY_SIZE){
			if(strncmp(data_block + j, filename, strlen(filename)+1)==0){//if we find the filename, return inode id
				int inode_id = data_block[j-1];//inode id preceding filename
				free(inode);
				free(data_block);
				fclose(vdisk);
				return inode_id; 
			}else if(((i-4)*BLOCK_SIZE + j + ENTRY_SIZE) > filesize){//if the next iteration will go past the filesize, return
				free(inode);
				free(data_block);
				fclose(vdisk);
				return -1;
			}
		}
	}
	// Note: indirect blocks are not used for directories (last two block pointers are direct block pointers)
	free(inode);
	free(data_block);
	fclose(vdisk);
	return -1;
}

/*
//returns an inode id for the file (plays the role of a file descriptor)
//if the file can be found in the path. If the file cannot be found, 
//a new file is created and its inode id is returned given that the
//lowest level directory in the path exists.
//If the function fails, it returns 0 (which is not an inode index) 
*/
int Open(char* pathname){
	
	if(pathname[0] != '/'){
		return 0;//all paths must start from the root
	}
	//store the separate filenames of the path
	char* path_buffer = (char*)malloc(strlen(pathname)+1);
	strncpy(path_buffer, pathname, strlen(pathname) + 1);
	const char* delim = "/";
	char* token = strtok(path_buffer, delim);
	int filename_count = 0;
	int capacity = sizeof(char*)*2;
	char** filenames = (char**)malloc(capacity);
	while( token != NULL ){
		filename_count++;
		if(sizeof(char*)*filename_count > capacity){
			capacity *= 2;
			filenames = (char**)realloc(filenames, capacity);
		}
		filenames[filename_count - 1] = token;
		token = strtok(NULL, delim);
	}
	
	int file = 1; //root directory's inode id
	int parent_dir;
	for(int i = 0; i < filename_count; i++){
		//search for filename
		parent_dir = file;
		file = search_for_filename(parent_dir, filenames[i]);
		
		if(file == -2){//error
			free(path_buffer);
			free(filenames);
			return 0;
		}
		
		if(file == -1){//filename not found
			if(i == filename_count - 1){//there are no filenames under this in the path
				int new_inode_id = create_at(parent_dir, filenames[i], 0 );
				free(path_buffer);
				free(filenames);
				return new_inode_id;
			}
			//otherwise, we were not able to search the whole path
			free(path_buffer);
			free(filenames);
			return 0;
		}
		
		if(file == 0){//the parent_dir is a file
			//this is a file along a path, which is not valid
			free(path_buffer);
			free(filenames);
			return 0;
		}
	}
	free(path_buffer);
	free(filenames);
	return file;
}


/*
//Create a directory in the path. Assume path up to directory exists.
//This function returns the new directory's inode id upon success and
//zero upon failure. If the directory of the new directory doesn't exist,
//this function creates a new file with its name, but this is the users fault
//as the function assumes the path exists up to the new directory.
*/
int Mkdir(char* pathname){
	int directory_length;
	for(int i = strlen(pathname)-1; i >= 0; i--){//search backwards for the end of the existing path
		if(pathname[i] == '/'){
			directory_length = i+1;
			break;
		}
	}
	
	char* dir_name = (char*)malloc(directory_length + 1);
	strncpy(dir_name, pathname, directory_length);
	dir_name[directory_length] = 0;
	int dir = Open(dir_name);
	free(dir_name);
	
	char* new_dir_name = (char*)malloc(strlen(pathname) - directory_length + 1);
	strncpy(new_dir_name, pathname + directory_length, strlen(pathname) - directory_length);
	new_dir_name[strlen(pathname) - directory_length] = 0;
	int new_dir = create_at(dir, new_dir_name, 1);
	free(new_dir_name);
	
	return new_dir;
}

/*
//Delete the directory specified by the path.
//This will not work on the root directory, or 
//in a directory that has files/subdirectories
//in it. Returns 0 on success and -1 on failure.
//Note: if the directory doesn't exist, it 
//creates a new file by that name (this is the 
//user's fault). You must specify a valid directory
//in order to delete it.
*/
int Rmdir(char* pathname){
	
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -1;
	}
	
	int fd = Open(pathname);
	if(fd == 0 || fd ==1){
		//failed to open or this is the root directory
		fclose(vdisk);
		return -1;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, fd + 9, inode)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	if(((int*)inode)[1] != 1){//not a directory
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	if(((int*)inode)[0] > ENTRY_SIZE*2){//there are more than two entries in it (not empty)
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	//The actions taken are the same for a file at this point
	//i.e. we look up the parent directory
	//and delete this file's entry in it, then we delete this file's
	//inode. Therefore, we simply call Unlink on the directory
	if(Unlink(pathname)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	//after this point the inode is actually deleted, but we have a copy
	
	void* blank_block = calloc(BLOCK_SIZE,1);
	
	void* free_block = malloc(BLOCK_SIZE);
	if(read_block(vdisk, 1, free_block)){
		free(blank_block);
		free(inode);
		free(free_block);
		fclose(vdisk);
		return -1;
	}
	
	set_bit(free_block, ((uint16_t*)inode)[4]); //free up the data block
	
	if(write_block(vdisk, 1, free_block)){
		free(free_block);
		free(inode);
		free(blank_block);
		fclose(vdisk);
		return -1;
	}
	free(free_block);
	
	if(write_block(vdisk, ((uint16_t*)inode)[4], blank_block)){
		free(blank_block);
		free(inode);
		fclose(vdisk);
		return -1;
	}
	free(blank_block);
	free(inode);
	
	fclose(vdisk);
	return 0;
}

/*
//Delete the file. Returns 0 on success, -1 on failure.
//For a file, this function will write 0 to the file
//(thereby deleting the data blocks). For both files/directories,
//it will look up the parent directory,
//delete its own entry in the directory,
//and delete its inode. This function assumes directory pathnames
//are coming from Rmdir, but still denies the right to delete the root.
//I'm not sure how Unlink is implemented elsewhere
//but I'm just taking it to mean delete, since that
//is the requirement in the assignment.
*/
int Unlink(char* pathname){
	
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -1;
	}
	
	int fd = Open(pathname);
	if(fd == 0 || fd ==1){
		//failed to open or this is the root directory
		fclose(vdisk);
		return -1;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, fd + 9, inode)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	int flag = ((int*)inode)[1];//store for later
	
	//OPEN PARENT DIRECTORY
	int directory_length;
	for(int i = strlen(pathname)-1; i >= 0; i--){
		if(pathname[i] == '/'){
			directory_length = i+1;
			break;
		}
	}
	
	char* dir_name = (char*)malloc(directory_length + 1);
	strncpy(dir_name, pathname, directory_length);
	dir_name[directory_length] = 0;
	int dir = Open(dir_name);
	free(dir_name);
	
	if(dir == 0){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	char* filename = pathname + directory_length;
	if(strlen(filename)> 30){
		//invalid filename
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	//STORE/ERASE LAST ENTRY FROM PARENT DIRECTORY
	void* dir_inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, dir + 9, dir_inode)){
		free(inode);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	
	int filesize = ((int*)dir_inode)[0];
	((int*)dir_inode)[0] -= ENTRY_SIZE;
	char* last_entry = malloc(ENTRY_SIZE);
	int file_block = filesize/BLOCK_SIZE;
	int file_entry = filesize%BLOCK_SIZE;
	if(file_entry == 0){
		file_block--;
		file_entry = BLOCK_SIZE;
	}
	char* last_block = (char*)malloc(BLOCK_SIZE);
	int last_block_num = ((uint16_t*)dir_inode)[file_block+4];
	if(read_block(vdisk, last_block_num, last_block)){
		free(inode);
		free(last_block);
		free(last_entry);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	memcpy(last_entry, last_block + file_entry - ENTRY_SIZE, ENTRY_SIZE);//store last entry
	memset(last_block + file_entry - ENTRY_SIZE, 0, ENTRY_SIZE);//erase last entry
	
	bool finished = false;
	int block_num;
	//SEARCH FOR ENTRY AND REPLACE IT WITH LAST ENTRY
	uint8_t* data_block = (uint8_t*)malloc(BLOCK_SIZE);
	for(int i = 4; i < INODE_SIZE/sizeof(uint16_t) && !finished; i++){//iterate through direct blocks
		if(read_block(vdisk, ((uint16_t*)dir_inode)[i], data_block)){
			free(inode);
			free(data_block);
			free(last_block);
			free(last_entry);
			free(dir_inode);
			fclose(vdisk);
			return -1;
		}
		for(int j = 0; j < BLOCK_SIZE; j+=ENTRY_SIZE){
			if(data_block[j] == (uint8_t)fd){//if we find the filename
				if(last_block_num == ((uint16_t*)dir_inode)[i]){//if this is the last block as well
					if(j != file_entry - ENTRY_SIZE){//we dont want to copy the last entry back in if we are deleting the last entry
						memcpy(last_block + j, last_entry, ENTRY_SIZE);
					}
				}else{
					memcpy(data_block + j, last_entry, ENTRY_SIZE);
				}
				free(last_entry);
				finished = true;
				block_num = ((uint16_t*)dir_inode)[i];
				break;
			}else if(((i-4)*BLOCK_SIZE + j + ENTRY_SIZE) > filesize){//weren't able to find it
				free(inode);
				free(data_block);
				free(last_block);
				free(last_entry);
				free(dir_inode);
				fclose(vdisk);
				return -1;
			}
		}
	}
	
	if(flag != 1){//not a directory
		if(write(fd, " ", 0)){//erase the file's data blocks (likely changes free_block)
			free(inode);
			free(data_block);
			free(last_block);
			free(dir_inode);
			fclose(vdisk);
			return -1;
		}
	}
	
	void* free_block = malloc(BLOCK_SIZE);
	if(read_block(vdisk, 1, free_block)){
		free(inode);
		free(free_block);
		free(data_block);
		free(last_block);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	//removing an entries requires us to move the 
	//last entry into the location of the removed
	//entry (maintains continuous data). It is 
	//therefore important to identify when we need
	//remove a data block from the directory
	//(in event there is one entry in last block)
	//this happens when file_entry == ENTRY_SIZE
	if(file_entry == ENTRY_SIZE){
		set_bit(free_block, ((uint16_t*)dir_inode)[file_block+4]);//free the block up
		((uint16_t*)dir_inode)[file_block+4] = 0;//remove block number
	}
	
	set_bit(free_block, fd + 9);//free up the inode
	memset(inode, 0 , BLOCK_SIZE);
	
	if(write_block(vdisk, 1, free_block)){
		free(inode);
		free(free_block);
		free(data_block);
		free(last_block);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	free(free_block);
	
	if(write_block(vdisk, fd + 9, inode)){//zero out old inode
		free(inode);
		free(data_block);
		free(last_block);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	free(inode);
	
	if(write_block(vdisk, dir + 9, dir_inode)){
		free(data_block);
		free(last_block);
		free(dir_inode);
		fclose(vdisk);
		return -1;
	}
	free(dir_inode);
	
	if(write_block(vdisk, block_num, data_block)){
		free(last_block);
		free(data_block);
		fclose(vdisk);
		return -1;
	}
	free(data_block);
	
	if(write_block(vdisk, last_block_num, last_block)){
		free(last_block);
		fclose(vdisk);
		return -1;
	}
	free(last_block);
	fclose(vdisk);
	return 0;
}

/*
//Reads up to length bytes from the file described
//by the given inode id (fd) into the buffer.
//If length >= filesize, then entire file is read.
//Returns 0 on success or -1 on failure.
//Note: It is the caller's responsibility to
//ensure the buffer is large enough to receive
//length bytes.
//Also, you cannot read a directory.
*/
int read(int fd, void* buffer, int length){
	if(fd < 1 || fd > NUM_INODES){
		return -1;//invalid inode id
	}
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -1;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, fd + 9, inode)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	if(((int*)inode)[1] == 1){//this file is a directory
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	int filesize = ((int*)inode)[0];
	if(filesize == 0){
		free(inode);
		fclose(vdisk);
		return 0;//nothing to read
	}
	
	int file_block;
	int file_byte;
	if(length < filesize){
		file_block = length/BLOCK_SIZE;
		file_byte = length%BLOCK_SIZE;
	}else{
		file_block = filesize/BLOCK_SIZE;
		file_byte = filesize%BLOCK_SIZE;		
	}
	if(file_byte == 0){
		file_byte = BLOCK_SIZE;
		file_block--;
	}
	
	int buffer_index = 0;
	char* data_block = (char*)malloc(BLOCK_SIZE);
	for(int i = 4; i < 14; i++){//iterate through direct blocks
		if(read_block(vdisk, ((uint16_t*)inode)[i], data_block)){
			free(inode);
			free(data_block);
			fclose(vdisk);
			return -1;
		}
		if( i-4 == file_block){//if we are on the last block
			memcpy((char*)buffer + buffer_index, data_block, file_byte);
			free(inode);
			free(data_block);
			fclose(vdisk);
			return 0;
		}else{
			memcpy((char*)buffer + buffer_index, data_block, BLOCK_SIZE);
			buffer_index += BLOCK_SIZE;
		}
	}
	
	uint16_t* single_indirect_block = (uint16_t*)malloc(BLOCK_SIZE);//pointers are two bytes
	if(read_block(vdisk, ((uint16_t*)inode)[14], single_indirect_block)){
		free(single_indirect_block);
		free(inode);
		free(data_block);
		fclose(vdisk);
		return -1;
	}
	for(int i = 0; i < BLOCK_SIZE/2; i++){
		if(read_block(vdisk, single_indirect_block[i], data_block)){
			free(single_indirect_block);
			free(inode);
			free(data_block);
			fclose(vdisk);
			return -1;
		}
		if( i+10 == file_block){//if we are on the last block
			memcpy((char*)buffer + buffer_index, data_block, file_byte);
			free(single_indirect_block);
			free(inode);
			free(data_block);
			fclose(vdisk);
			return 0;
		}else{
			memcpy((char*)buffer + buffer_index, data_block, BLOCK_SIZE);
			buffer_index += BLOCK_SIZE;
		}
	}
	
	uint16_t* double_indirect_block = (uint16_t*)malloc(BLOCK_SIZE);//pointers are two bytes
	if(read_block(vdisk, ((uint16_t*)inode)[15], double_indirect_block)){
		free(double_indirect_block);
		free(single_indirect_block);
		free(inode);
		free(data_block);
		fclose(vdisk);
		return -1;
	}
	for(int i = 0; i < BLOCK_SIZE/2; i++){
		if(read_block(vdisk, double_indirect_block[i], single_indirect_block)){
			free(double_indirect_block);
			free(single_indirect_block);
			free(inode);
			free(data_block);
			fclose(vdisk);
			return -1;
		}
		for(int j = 0; j < BLOCK_SIZE/2; j++){
			if(read_block(vdisk, single_indirect_block[j], data_block)){
				free(double_indirect_block);
				free(single_indirect_block);
				free(inode);
				free(data_block);
				fclose(vdisk);
				return -1;
			}
			if( j+266 + i*256 == file_block){//if we are on the last block
				memcpy((char*)buffer + buffer_index, data_block, file_byte);
				free(double_indirect_block);
				free(single_indirect_block);
				free(inode);
				free(data_block);
				fclose(vdisk);
				return 0;
			}else{
				memcpy((char*)buffer + buffer_index, data_block, BLOCK_SIZE);
				buffer_index += BLOCK_SIZE;
			}
		}
	}
	
	free(double_indirect_block);
	free(single_indirect_block);
	free(inode);
	free(data_block);
	fclose(vdisk);
	return 0;
}

/*
//WARNING: THIS FUNCTION IS DISGUSTINGLY LONG (I HATE IT TOO, I JUST DONT HAVE TIME TO MAKE IT BETTER)
//
//Writes length bytes from buffer into the file described
//by the given inode id (fd). Returns 0 on success or -1 on failure.
//Also, you cannot write to a directory.
//Note: It is the caller's responsibility to
//ensure the buffer is large enough to write
//length bytes.
//
//This was a monster to write, I realized later that I probably
//shouldn't have made it go through and erase everything else in
//the old file, but it's good security wise, because old data
//doesn't linger on the disk. It also reuses the old data blocks
//of the original file
*/
int write(int fd, void* buffer, int length){
	if(fd < 1 || fd > NUM_INODES){
		return -1;//invalid inode id
	}
	if(length > (NUM_BLOCKS - 11 - NUM_INODES)*BLOCK_SIZE || length < 0){// -11 comes from reserved blocks and root directory's first data block
		return -1; //definitely cannot support a file this size
	}
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return -1;
	}
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, fd + 9, inode)){
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	if(((int*)inode)[1] == 1){//this file is a directory
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	int filesize = ((int*)inode)[0];
	((int*)inode)[0] = length;//new filesize
	
	if(filesize == 0 && length == 0){
		free(inode);
		fclose(vdisk);
		return -1;//no changes are necessary
	}
	
	int file_block;//used to indicate the index of the final block in common between the old file data and new
	int file_byte;//used to indicate the number of bytes in common between the old file data and new in the last common file block
	int diff;//difference between old filesize and new length
	bool grow_file = false;//indicates whether we are erasing the difference or writing the difference
	bool finished = false;//lets us know when we are done
	if(length <= filesize){
		file_block = length/BLOCK_SIZE;
		file_byte = length%BLOCK_SIZE;
		diff = filesize - length;
	}else{
		file_block = filesize/BLOCK_SIZE;
		file_byte = filesize%BLOCK_SIZE;
		grow_file = true;
		diff = length - filesize;
	}
	
	int capacity = sizeof(char*)*(file_block + 1); 
	char** data_blocks = (char**)malloc(capacity); //use this to store all of our changes so we can write them all at the same time.
	int* data_block_ids = (int*)malloc(capacity); //maintain the index of each block we modify (in case we erase the inode's references to them)
	int db_index = 0;
	
	void* free_block = malloc(BLOCK_SIZE);
	if(read_block(vdisk, 1, free_block)){
		free(free_block);
		free(data_blocks);
		free(data_block_ids);
		free(inode);
		fclose(vdisk);
		return -1;
	}
	
	if(file_byte == 0 && file_block != 0){//the common data fills up a certain number of blocks exactly
		file_byte = BLOCK_SIZE;
		file_block--;
	}else if(file_byte ==0 && file_block == 0 && length > 0){//we need to reserve the first block
		int block_id = available_data_block(vdisk, free_block);
		if(block_id == -1){//unable to reserve a data block
			free(free_block);
			free(inode);
			free(data_blocks);
			free(data_block_ids);
			fclose(vdisk);
			return -1;
		}
		unset_bit(free_block, block_id);
		((uint16_t*)inode)[4] = block_id;
	}
	
	int buffer_index = 0;
	for(int i = 4; i < 14 && !finished; i++){//iterate through direct blocks
		char* data_block = (char*)malloc(BLOCK_SIZE);
		if(read_block(vdisk, ((uint16_t*)inode)[i], data_block)){
			free(data_block);
			for(int k = 0; k < db_index; k++){
				free(data_blocks[k]);
			}
			free(data_blocks);
			free(free_block);
			free(data_block_ids);
			free(inode);
			fclose(vdisk);
			return -1;
		}
		
		if(sizeof(char*)*db_index == capacity){
			capacity *= 2;
			data_blocks = (char**)realloc(data_blocks, capacity);
			data_block_ids = (int*)realloc(data_block_ids, capacity);
		}
		data_blocks[db_index] = data_block;//keep a reference to the block
		data_block_ids[db_index] = ((uint16_t*)inode)[i];//keep its block number
		db_index++;
		
		if( i-4 == file_block){//if we are on the last block in common
			memcpy(data_block, (char*)buffer + buffer_index, file_byte);
			buffer_index += file_byte;
			if(grow_file){
				if(diff > BLOCK_SIZE - file_byte){
					memcpy(data_block + file_byte, (char*)buffer + buffer_index, BLOCK_SIZE - file_byte);
					buffer_index += BLOCK_SIZE - file_byte;
					diff -= (BLOCK_SIZE - file_byte);
					
					//need to reserve the next data block
					int block_id = available_data_block(vdisk, free_block);
					if(block_id == -1){//unable to reserve a data block
						for(int k = 0; k < db_index; k++){
							free(data_blocks[k]);
						}
						free(data_blocks);
						free(data_block_ids);
						free(free_block);
						free(inode);
						fclose(vdisk);
						return -1;
					}
					unset_bit(free_block, block_id);
					((uint16_t*)inode)[i+1] = block_id;
					
				}else{
					memcpy(data_block + file_byte, (char*)buffer + buffer_index, diff);
					diff = 0;
					finished = true;
				}
			}else{//fill remainder with zeros
				if(diff > BLOCK_SIZE - file_byte){
					memset(data_block + file_byte, 0, BLOCK_SIZE - file_byte);
					diff -= (BLOCK_SIZE - file_byte);
					if(length == 0){//if we are erasing everything
						set_bit(free_block, ((uint16_t*)inode)[i]);
						((uint16_t*)inode)[i] = 0;
					}
				}else{
					memset(data_block + file_byte, 0, diff);
					diff = 0;
					if(length == 0){//if we are erasing everything
						set_bit(free_block, ((uint16_t*)inode)[i]);
						((uint16_t*)inode)[i] = 0;
					}
					finished = true;
				}
			}
		}else if(i-4 < file_block){//continue overwriting blocks in common
			memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
			buffer_index += BLOCK_SIZE;
		}else{
			if(grow_file){// continue writing data until diff is 0
				if(diff > BLOCK_SIZE){
					memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
					buffer_index += BLOCK_SIZE;
					diff -= BLOCK_SIZE;
					
					//need to reserve the next data block
					int block_id = available_data_block(vdisk, free_block);
					if(block_id == -1){//unable to reserve a data block
						for(int k = 0; k < db_index; k++){
							free(data_blocks[k]);
						}
						free(data_blocks);
						free(data_block_ids);
						free(free_block);
						free(inode);
						fclose(vdisk);
						return -1;
					}
					unset_bit(free_block, block_id);
					((uint16_t*)inode)[i+1] = block_id;
					
				}else{
					memcpy(data_block, (char*)buffer + buffer_index, diff);
					diff = 0;
					finished = true;
				}
			}else{//continue erasing data until diff is 0
				if(diff > BLOCK_SIZE){
					memset(data_block, 0, BLOCK_SIZE);
					diff -= BLOCK_SIZE;
					//free up this block
					set_bit(free_block, ((uint16_t*)inode)[i]);
					((uint16_t*)inode)[i] = 0;
				}else{
					memset(data_block, 0, diff);
					diff = 0;
					//free up this block
					set_bit(free_block, ((uint16_t*)inode)[i]);
					((uint16_t*)inode)[i] = 0;
					finished = true;
				}
			}
		}
	}
	
	bool affected_single_indirects = false;
	bool affected_double_indirect = false;
	uint16_t* double_indirect_block = (uint16_t*)malloc(BLOCK_SIZE);
	uint16_t** si_blocks = (uint16_t**)malloc(sizeof(uint16_t*)*(BLOCK_SIZE/2 + 1));//there are a maximum of 257 single_indirect_blocks
	int* si_block_ids = (int*)malloc(sizeof(int*)*257);
	int si_index = 0;
	
	if(!finished){//SINGLE INDIRECT BLOCK
		
		affected_single_indirects = true;
		uint16_t* single_indirect_block = (uint16_t*)malloc(BLOCK_SIZE);
		if(read_block(vdisk, ((uint16_t*)inode)[14], single_indirect_block)){//we know inode[14] is safe because we would have allocated it above
			free(single_indirect_block);
			for(int k = 0; k < db_index; k++){
				free(data_blocks[k]);
			}
			free(data_blocks);
			free(data_block_ids);
			free(si_blocks);
			free(si_block_ids);
			free(double_indirect_block);
			free(free_block);
			free(inode);
			fclose(vdisk);
			return -1;
		}
		si_blocks[si_index] = single_indirect_block;
		si_block_ids[si_index] = ((uint16_t*)inode)[14];
		si_index++;
		
		if(file_block < 10 && grow_file){
			//need to reserve another block (file_block indicates where old data stopped,
			//and grow_file indicates that data blocks are not reserved for where we want to write the file.
			int block_id = available_data_block(vdisk, free_block);
			if(block_id == -1){//unable to reserve a data block
				for(int k = 0; k < db_index; k++){
					free(data_blocks[k]);
				}
				free(data_blocks);
				free(data_block_ids);
				for(int k = 0; k < si_index; k++){
					free(si_blocks[k]);
				}
				free(si_blocks);
				free(si_block_ids);
				free(double_indirect_block);
				free(free_block);
				free(inode);
				fclose(vdisk);
				return -1;
			}
			unset_bit(free_block, block_id);
			single_indirect_block[0] = block_id;
		}if(file_block < 10 && !grow_file){//we must free the single_indirect_block
			set_bit(free_block, ((uint16_t*)inode)[14]);
			((uint16_t*)inode)[14] = 0;
		}
		
		for(int i = 0; i < BLOCK_SIZE/2 && !finished; i++){
			
			char* data_block = (char*)malloc(BLOCK_SIZE);
			if(read_block(vdisk, single_indirect_block[i], data_block)){
				free(single_indirect_block);
				for(int k = 0; k < db_index; k++){
					free(data_blocks[k]);
				}
				free(data_blocks);
				free(data_block_ids);
				free(si_blocks);
				free(si_block_ids);
				free(double_indirect_block);
				free(free_block);
				free(inode);
				fclose(vdisk);
				return -1;
			}
			
			if(sizeof(char*)*db_index == capacity){
				capacity *= 2;
				data_blocks = (char**)realloc(data_blocks, capacity);
				data_block_ids = (int*)realloc(data_block_ids, capacity);
			}
			data_blocks[db_index] = data_block;//keep a reference to the block
			data_block_ids[db_index] = single_indirect_block[i];//keep its block number
			db_index++;
			
			if( i+10 == file_block){//if we are on the last block in common
				memcpy(data_block, (char*)buffer + buffer_index, file_byte);
				buffer_index += file_byte;
				if(grow_file){
					if(diff > BLOCK_SIZE - file_byte){
						memcpy(data_block + file_byte, (char*)buffer + buffer_index, BLOCK_SIZE - file_byte);
						buffer_index += BLOCK_SIZE - file_byte;
						diff -= (BLOCK_SIZE - file_byte);
						
						//need to reserve the next data block
						int block_id = available_data_block(vdisk, free_block);
						if(block_id == -1){//unable to reserve a data block
							free(single_indirect_block);
							for(int k = 0; k < db_index; k++){
								free(data_blocks[k]);
							}
							free(data_blocks);
							free(data_block_ids);
							free(si_blocks);
							free(si_block_ids);
							free(double_indirect_block);
							free(free_block);
							free(inode);
							fclose(vdisk);
							return -1;
						}
						unset_bit(free_block, block_id);
						if(i < BLOCK_SIZE/2 -1){
							single_indirect_block[i+1] = block_id;
						}else{
							((uint16_t*)inode)[15] = block_id;
						}
						
					}else{
						memcpy(data_block + file_byte, (char*)buffer + buffer_index, diff);
						diff = 0;
						finished = true;
					}
				}else{//fill remainder with zeros
					if(diff > BLOCK_SIZE - file_byte){
						memset(data_block + file_byte, 0, BLOCK_SIZE - file_byte);
						diff -= (BLOCK_SIZE - file_byte);
					}else{
						memset(data_block + file_byte, 0, diff);
						diff = 0;
						finished = true;
					}
				}
			}else if(i+10 < file_block){//continue overwriting blocks in common
				memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
				buffer_index += BLOCK_SIZE;
			}else{
				if(grow_file){// continue writing data until diff is 0
					if(diff > BLOCK_SIZE){
						memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
						buffer_index += BLOCK_SIZE;
						diff -= BLOCK_SIZE;
						
						//need to reserve the next data block
						int block_id = available_data_block(vdisk, free_block);
						if(block_id == -1){//unable to reserve a data block
							free(single_indirect_block);
							for(int k = 0; k < db_index; k++){
								free(data_blocks[k]);
							}
							free(data_blocks);
							free(data_block_ids);
							free(si_blocks);
							free(si_block_ids);
							free(double_indirect_block);
							free(free_block);
							free(inode);
							fclose(vdisk);
							return -1;
						}
						unset_bit(free_block, block_id);
						if(i < BLOCK_SIZE/2 -1){
							single_indirect_block[i+1] = block_id;
						}else{
							((uint16_t*)inode)[15] = block_id;
						}
						
					}else{
						memcpy(data_block, (char*)buffer + buffer_index, diff);
						diff = 0;
						finished = true;
					}
				}else{//continue erasing data until diff is 0
					if(diff > BLOCK_SIZE){
						memset(data_block, 0, BLOCK_SIZE);
						diff -= BLOCK_SIZE;
						//free up this block
						set_bit(free_block, single_indirect_block[i]);
						single_indirect_block[i] = 0;
					}else{
						memset(data_block, 0, diff);
						diff = 0;
						//free up this block
						set_bit(free_block, single_indirect_block[i]);
						single_indirect_block[i] = 0;
						finished = true;
					}
				}
			}
		}
		
		if(!finished){//DOUBLE INDIRECT BLOCK
			
			affected_double_indirect = true;
			//we know inode[15] is safe because we would have allocated it above
			if(read_block(vdisk, ((uint16_t*)inode)[15], double_indirect_block)){
				for(int k = 0; k < db_index; k++){
					free(data_blocks[k]);
				}
				free(data_blocks);
				free(data_block_ids);
				for(int k = 0; k < si_index; k++){
					free(si_blocks[k]);
				}
				free(si_blocks);
				free(si_block_ids);
				free(double_indirect_block);
				free(free_block);
				free(inode);
				fclose(vdisk);
				return -1;
			}
			
			if(file_block < 266 && grow_file){
				//need to reserve another block (file_block indicates where old data stopped,
				//and grow_file indicates that data blocks are not reserved for where we want to write the file.
				int block_id = available_data_block(vdisk, free_block);
				if(block_id == -1){//unable to reserve a data block
					for(int k = 0; k < db_index; k++){
						free(data_blocks[k]);
					}
					free(data_blocks);
					free(data_block_ids);
					for(int k = 0; k < si_index; k++){
						free(si_blocks[k]);
					}
					free(si_blocks);
					free(si_block_ids);
					free(double_indirect_block);
					free(free_block);
					free(inode);
					fclose(vdisk);
					return -1;
				}
				unset_bit(free_block, block_id);
				double_indirect_block[0] = block_id;
			}if(file_block < 266 && !grow_file){//we must free the double_indirect_block
				set_bit(free_block, ((uint16_t*)inode)[15]);
				((uint16_t*)inode)[15] = 0;
			}
			
			for(int p = 0; p < BLOCK_SIZE/2 && !finished; p++){
				
				uint16_t* single_indirect_block = (uint16_t*)malloc(BLOCK_SIZE);
				if(read_block(vdisk, double_indirect_block[p], single_indirect_block)){
					for(int k = 0; k < db_index; k++){
						free(data_blocks[k]);
					}
					free(data_blocks);
					free(data_block_ids);
					for(int k = 0; k < si_index; k++){
						free(si_blocks[k]);
					}
					free(si_blocks);
					free(si_block_ids);
					free(double_indirect_block);
					free(free_block);
					free(inode);
					fclose(vdisk);
					return -1;
				}
				si_blocks[si_index] = single_indirect_block;
				si_block_ids[si_index] = double_indirect_block[p];
				si_index++;
				
				if(file_block < 266 + p*256 && grow_file){
					//need to reserve another block (file_block indicates where old data stopped,
					//and grow_file indicates that data blocks are not reserved for where we want to write the file.
					int block_id = available_data_block(vdisk, free_block);
					if(block_id == -1){//unable to reserve a data block
						for(int k = 0; k < db_index; k++){
							free(data_blocks[k]);
						}
						free(data_blocks);
						free(data_block_ids);
						for(int k = 0; k < si_index; k++){
							free(si_blocks[k]);
						}
						free(si_blocks);
						free(si_block_ids);
						free(double_indirect_block);
						free(free_block);
						free(inode);
						fclose(vdisk);
						return -1;
					}
					unset_bit(free_block, block_id);
					single_indirect_block[0] = block_id;
				}if(file_block < 266 + p*256 && !grow_file){//we must free the single_indirect_block
					set_bit(free_block, double_indirect_block[p]);
					double_indirect_block[p] = 0;
				}
				
				for(int i = 0; i < BLOCK_SIZE/2 && !finished; i++){
					
					char* data_block = (char*)malloc(BLOCK_SIZE);
					if(read_block(vdisk, single_indirect_block[i], data_block)){
						for(int k = 0; k < db_index; k++){
							free(data_blocks[k]);
						}
						free(data_blocks);
						free(data_block_ids);
						for(int k = 0; k < si_index; k++){
							free(si_blocks[k]);
						}
						free(si_blocks);
						free(si_block_ids);
						free(double_indirect_block);
						free(free_block);
						free(inode);
						fclose(vdisk);
						return -1;
					}
					
					if(sizeof(char*)*db_index == capacity){
						capacity *= 2;
						data_blocks = (char**)realloc(data_blocks, capacity);
						data_block_ids = (int*)realloc(data_block_ids, capacity);
					}
					data_blocks[db_index] = data_block;//keep a reference to the block
					data_block_ids[db_index] = single_indirect_block[i];//keep its block number
					db_index++;
					
					if( i+ 266 + p*256 == file_block){//if we are on the last block in common
						memcpy(data_block, (char*)buffer + buffer_index, file_byte);
						buffer_index += file_byte;
						if(grow_file){
							if(diff > BLOCK_SIZE - file_byte){
								memcpy(data_block + file_byte, (char*)buffer + buffer_index, BLOCK_SIZE - file_byte);
								buffer_index += BLOCK_SIZE - file_byte;
								diff -= (BLOCK_SIZE - file_byte);
								
								//need to reserve the next data block
								int block_id = available_data_block(vdisk, free_block);
								if(block_id == -1){//unable to reserve a data block
									for(int k = 0; k < db_index; k++){
										free(data_blocks[k]);
									}
									free(data_blocks);
									free(data_block_ids);
									for(int k = 0; k < si_index; k++){
										free(si_blocks[k]);
									}
									free(si_blocks);
									free(si_block_ids);
									free(double_indirect_block);
									free(free_block);
									free(inode);
									fclose(vdisk);
									return -1;
								}
								unset_bit(free_block, block_id);
								if(i < BLOCK_SIZE/2 -1){
									single_indirect_block[i+1] = block_id;
								}else if(p < BLOCK_SIZE/2 -1){
									double_indirect_block[p+1] = block_id;
								}else{//RAN OUT OF ROOM
									for(int k = 0; k < db_index; k++){
										free(data_blocks[k]);
									}
									free(data_blocks);
									free(data_block_ids);
									for(int k = 0; k < si_index; k++){
										free(si_blocks[k]);
									}
									free(si_blocks);
									free(si_block_ids);
									free(double_indirect_block);
									free(free_block);
									free(inode);
									fclose(vdisk);
									return -1;
								}
								
							}else{
								memcpy(data_block + file_byte, (char*)buffer + buffer_index, diff);
								diff = 0;
								finished = true;
							}
						}else{//fill remainder with zeros
							if(diff > BLOCK_SIZE - file_byte){
								memset(data_block + file_byte, 0, BLOCK_SIZE - file_byte);
								diff -= (BLOCK_SIZE - file_byte);
							}else{
								memset(data_block + file_byte, 0, diff);
								diff = 0;
								finished = true;
							}
						}
					}else if(i+ 266 + p*256 < file_block){//continue overwriting blocks in common
						memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
						buffer_index += BLOCK_SIZE;
					}else{
						if(grow_file){// continue writing data until diff is 0
							if(diff > BLOCK_SIZE){
								memcpy(data_block, (char*)buffer + buffer_index, BLOCK_SIZE);
								buffer_index += BLOCK_SIZE;
								diff -= BLOCK_SIZE;
								
								//need to reserve the next data block
								int block_id = available_data_block(vdisk, free_block);
								if(block_id == -1){//unable to reserve a data block
									for(int k = 0; k < db_index; k++){
										free(data_blocks[k]);
									}
									free(data_blocks);
									free(data_block_ids);
									for(int k = 0; k < si_index; k++){
										free(si_blocks[k]);
									}
									free(si_blocks);
									free(si_block_ids);
									free(double_indirect_block);
									free(free_block);
									free(inode);
									fclose(vdisk);
									return -1;
								}
								unset_bit(free_block, block_id);
								if(i < BLOCK_SIZE/2 -1){
									single_indirect_block[i+1] = block_id;
								}else if(p < BLOCK_SIZE/2 -1){
									double_indirect_block[p+1] = block_id;
								}else{//RAN OUT OF ROOM
									for(int k = 0; k < db_index; k++){
										free(data_blocks[k]);
									}
									free(data_blocks);
									free(data_block_ids);
									for(int k = 0; k < si_index; k++){
										free(si_blocks[k]);
									}
									free(si_blocks);
									free(si_block_ids);
									free(double_indirect_block);
									free(free_block);
									free(inode);
									fclose(vdisk);
									return -1;
								}
								
							}else{
								memcpy(data_block, (char*)buffer + buffer_index, diff);
								diff = 0;
								finished = true;
							}
						}else{//continue erasing data until diff is 0
							if(diff > BLOCK_SIZE){
								memset(data_block, 0, BLOCK_SIZE);
								diff -= BLOCK_SIZE;
								//free up this block
								set_bit(free_block, single_indirect_block[i]);
								single_indirect_block[i] = 0;
							}else{
								memset(data_block, 0, diff);
								diff = 0;
								//free up this block
								set_bit(free_block, single_indirect_block[i]);
								single_indirect_block[i] = 0;
								finished = true;
							}
						}
					}
				}
			}
		}
	}
	
	if(write_block(vdisk, 1, free_block)){//reserved / freed data blocks
		for(int k = 0; k < db_index; k++){
			free(data_blocks[k]);
		}
		free(data_blocks);
		free(data_block_ids);
		for(int k = 0; k < si_index; k++){
			free(si_blocks[k]);
		}
		free(si_blocks);
		free(si_block_ids);
		free(double_indirect_block);
		free(free_block);
		free(inode);
		fclose(vdisk);
		return -1;
	}
	free(free_block);
	
	//CRASH COULD OCCUR HERE
	
	if(write_block(vdisk, fd + 9, inode)){
		for(int k = 0; k < db_index; k++){
			free(data_blocks[k]);
		}
		free(data_blocks);
		free(data_block_ids);
		for(int k = 0; k < si_index; k++){
			free(si_blocks[k]);
		}
		free(si_blocks);
		free(si_block_ids);
		free(double_indirect_block);
		free(inode);
		fclose(vdisk);
		return -1;
	}
	int di_block_num = ((uint16_t*)inode)[15];
	free(inode);
	
	//CRASH COULD OCCUR HERE
	
	if(affected_double_indirect){
		if(write_block(vdisk, di_block_num, double_indirect_block)){
			for(int k = 0; k < db_index; k++){
				free(data_blocks[k]);
			}
			free(data_blocks);
			free(data_block_ids);
			for(int k = 0; k < si_index; k++){
				free(si_blocks[k]);
			}
			free(si_blocks);
			free(si_block_ids);
			free(double_indirect_block);
			fclose(vdisk);
			return -1;
		}
	}
	free(double_indirect_block);
	
	//CRASH COULD OCCUR HERE
	
	if(affected_single_indirects){
		for(int k = 0; k < si_index; k++){
			if(write_block(vdisk, si_block_ids[k], si_blocks[k])){
				for(int j = k; j < si_index; k++){
					free(si_blocks[j]);
				}
				for(int k = 0; k < db_index; k++){
					free(data_blocks[k]);
				}
				free(data_blocks);
				free(data_block_ids);
				free(si_blocks);
				free(si_block_ids);
				fclose(vdisk);
				return -1;
			}
			free(si_blocks[k]);
		}
	}
	free(si_blocks);
	free(si_block_ids);
	
	//CRASH COULD OCCUR HERE
	
	for(int k = 0; k < db_index; k++){
		if(write_block(vdisk, data_block_ids[k], data_blocks[k])){
			for(int j = k; j < db_index; k++){
				free(data_blocks[j]);
			}
			free(data_block_ids);
			free(data_blocks);
			fclose(vdisk);
			return -1;
		}
		free(data_blocks[k]);
		
		//CRASH COULD OCCUR HERE
	}
	free(data_blocks);
	free(data_block_ids);
	fclose(vdisk);
	return 0;
}

/*void walk_through_inode(FILE* vdisk, void* free_block, int id){
	
	void* inode = malloc(BLOCK_SIZE);
	if(read_block(vdisk, id+9, inode)){
		free(inode);
		return;
	}
	bool finished = false;
	for(int i = 4; i < 14 && !finished; i++){
		if(((uint16_t*)inode)[i] == 0){
			finished = true;
		}else if(check_bit(free_block,((uint16_t*)inode)[i]){
			if(block_is_empty(vdisk,((uint16_t*)inode)[i])){
				((uint16_t*)inode)[i] = 0;//should not be in inode
			}else{
				unset_bit(free_block, ((uint16_t*)inode)[i]);
			}
		}else{
			if(block_is_empty(vdisk,((uint16_t*)inode)[i])){
				set_bit(free_block, ((uint16_t*)inode)[i]);
				((uint16_t*)inode)[i] = 0;//should not be in inode
			}
		}
	}
	
	void* si_block = malloc(BLOCK_SIZE);
	if(read_block(vdisk, ((uint16_t*)inode)[14], si_block)){
		free(si_block);
		free(inode);
		return;
	}
	for(int i = 0; i < 256 && !finished; i++){
		if(((uint16_t*)si_block)[i] == 0){
			finished = true;
		}else if(check_bit(free_block,((uint16_t*)si_block)[i]){
			if(block_is_empty(vdisk,((uint16_t*)si_block)[i])){
				((uint16_t*)si_block)[i] = 0;//should not be in si_block
			}else{
				unset_bit(free_block, ((uint16_t*)si_block)[i]);
			}
		}else{
			if(block_is_empty(vdisk,((uint16_t*)si_block)[i])){
				set_bit(free_block, ((uint16_t*)si_block)[i]);
				((uint16_t*)si_block)[i] = 0;//should not be in si_block
			}
		}
	}
	
	void* di_block = malloc(BLOCK_SIZE);
	if(read_block(vdisk, ((uint16_t*)inode)[15], di_block)){
		free(di_block);
		free(si_block);
		free(inode);
		return;
	}
	
	for(int p = 0; p < 256 && !finished; p++){
		if(read_block(vdisk, ((uint16_t*)di_block)[p], si_block)){
			free(di_block);
			free(si_block);
			free(inode);
			return;
		}
		for(int i = 0; i < 256 && !finished; i++){
			if(((uint16_t*)si_block)[i] == 0){
				finished = true;
			}else if(check_bit(free_block,((uint16_t*)si_block)[i]){
				if(block_is_empty(vdisk,((uint16_t*)si_block)[i])){
					((uint16_t*)si_block)[i] = 0;//should not be in si_block
				}else{
					unset_bit(free_block, ((uint16_t*)si_block)[i]);
				}
			}else{
				if(block_is_empty(vdisk,((uint16_t*)si_block)[i])){
					set_bit(free_block, ((uint16_t*)si_block)[i]);
					((uint16_t*)si_block)[i] = 0;//should not be in si_block
				}
			}
		}
	}
}*/

void fsck(){
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return;
	}
	
	void* free_block= malloc(BLOCK_SIZE);
	if(read_block(vdisk, 1, free_block)){
		free(free_block);
		fclose(vdisk);
		return;
	}
	
	//walk_through_inode(vdisk, free_block, 1);
	//start walk at root
	//the function would have checked if the inode was a directory
	//if so, call walk_through_dir, where it would then call walk_through_inode
	//on each of the inodes inside it. walk_through_inode itself would be iterative.
	for(int i = 0; i < NUM_BLOCKS; i++){
		if(check_bit(free_block, i)){
			if(!block_is_empty(vdisk, i)){
				unset_bit(free_block, i);//include the possibly corrupt data
			}
		}else{
			if(block_is_empty(vdisk, i)){
				set_bit(free_block, i);//free the block as it has nothing in it
			}
		}
	}
	free(free_block);
	fclose(vdisk);
}

/*
//Formats the vdisk with LLFS.
//Creates the superblock, freeblock,
//root inode, and its first entry.
*/
void init_LLFS(){
	init_vdisk();
	FILE* vdisk = fopen("../disk/vdisk", "rb+");
	if(vdisk == NULL){
		fprintf(stderr, "\nERROR:\nUnable to open vdisk\n");
		return;
	}
	
	//SUPERBLOCK
	int* super_block = (int*)calloc(BLOCK_SIZE, 1);
	int magic_number = 0xFFABCDEF; //arbitrary magic number for my file system
	super_block[0] = magic_number;
	super_block[1] = NUM_BLOCKS;//not certain if this number should include/exclude number of inodes
	super_block[2] = NUM_INODES;//this number will be used to index inodes in the inode area
	if(write_block(vdisk, 0, (char*)super_block)){
		free(super_block);
		return;
	}
	free(super_block);
	
	//FREEBLOCK
	uint8_t* free_block = (uint8_t*)calloc(BLOCK_SIZE, 1);
	for(int i = 0; i < BLOCK_SIZE; i++){
		free_block[i] = 0xFF;//initialize to ones to show every block is free
	}
	free_block[0] = 0x00;
	free_block[1] = 0x1F; //reserve blocks 0-9, and block 10 for inode 1
	unset_bit(free_block, NUM_INODES + 10);//reserve a data block for the root directory
	if(write_block(vdisk, 1, free_block)){
		free(free_block);
		return;
	}
	free(free_block);
	
	//write the inode + data block for the root
	uint8_t* data_block = (uint8_t*)calloc(BLOCK_SIZE,1);
	data_block[0] = 0x01; //root's inode value
	data_block[1] = '.';
	if(write_block(vdisk, NUM_INODES + 10, data_block)){
		free(data_block);
		return;
	}
	free(data_block);
	
	void* inode = calloc(BLOCK_SIZE,1);
	int filesize = ENTRY_SIZE;
	((int*)inode)[0] = filesize;
	((int*)inode)[1] = 1; //this file is a directory
	((uint16_t*)inode)[4] = NUM_INODES + 10; //block number for first data block
	if(write_block(vdisk, 10, inode)){
		free(inode);
		return;
	}
	free(inode);
	
	fclose(vdisk);
}
