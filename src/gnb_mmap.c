#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#ifdef __UNIX_LIKE_OS__
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <limits.h>

#include "gnb_mmap.h"

typedef struct _gnb_mmap_block_t {

#ifdef __UNIX_LIKE_OS__
	int fd;
#endif

#ifdef _WIN32
	HANDLE file_descriptor;
	HANDLE map_handle;
#endif

	char filename[PATH_MAX];
	void *block;
	size_t block_size;
	int mmap_type;

}gnb_mmap_block_t;


#ifdef __UNIX_LIKE_OS__

gnb_mmap_block_t* gnb_mmap_create(const char *filename, size_t block_size, int mmap_type){

	gnb_mmap_block_t *mmap_block;

	int fd;

	int oflag;

	int prot;

	if ( mmap_type & GNB_MMAP_TYPE_READWRITE){
		oflag = O_RDWR|O_CREAT;
	}else{
		oflag = O_RDONLY;
	}

	fd = open (filename, oflag, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	if ( -1 == fd ){
		return NULL;
	}

	if ( mmap_type & GNB_MMAP_TYPE_CREATE){
		if ( -1 == ftruncate(fd,block_size) ) {
			close(fd);
			return NULL;
		}
	}

	mmap_block = (gnb_mmap_block_t *)malloc(sizeof(gnb_mmap_block_t));

	snprintf(mmap_block->filename, PATH_MAX, "%s", filename);

	mmap_block->fd = fd;
	mmap_block->block_size = block_size;
	mmap_block->mmap_type = mmap_type;

	if ( mmap_type & GNB_MMAP_TYPE_READWRITE){
		prot = PROT_READ|PROT_WRITE;
	}else{
		prot = PROT_READ;
	}

	mmap_block->block = mmap(NULL, block_size, prot, MAP_SHARED, mmap_block->fd, 0);

	if ( mmap_type & GNB_MMAP_TYPE_CREATE){
		memset(mmap_block->block,0,block_size);
	}


	return mmap_block;

}

void gnb_mmap_release(gnb_mmap_block_t *mmap_block){

	munmap(mmap_block->block,mmap_block->block_size);

	close(mmap_block->fd);

	if ( mmap_block->mmap_type & (GNB_MMAP_TYPE_READWRITE|GNB_MMAP_TYPE_CLEANEXIT) ){
		unlink(mmap_block->filename);
	}

	free(mmap_block);

}

#endif



#ifdef _WIN32
#include "gnb_binary.h"
gnb_mmap_block_t* gnb_mmap_create(const char *filename, size_t block_size, int mmap_type){

	char mapping_buffer[PATH_MAX];

	char *mapping_name;

	mapping_name = gnb_bin2hex_string((void *)filename, strlen(filename), mapping_buffer);

	if ( NULL==mapping_name ){
		return NULL;
	}

	gnb_mmap_block_t *mmap_block;

	int oflag1;
	int oflag2;
	int oflag3;
	int oflag4;

	int prot;

	if ( mmap_type & GNB_MMAP_TYPE_READWRITE){
		oflag1 = GENERIC_READ | GENERIC_WRITE;
		oflag2 = FILE_SHARE_READ | FILE_SHARE_WRITE;
		oflag3 = OPEN_ALWAYS;
		oflag4 = PAGE_READWRITE;
		prot   = FILE_MAP_WRITE  | FILE_MAP_READ;
	}else{
		oflag1 = GENERIC_READ    | GENERIC_WRITE;
		oflag2 = FILE_SHARE_READ | FILE_SHARE_WRITE;
		oflag3 = OPEN_EXISTING;
		oflag4 = PAGE_READWRITE;
		prot   = FILE_MAP_READ;
	}

	HANDLE file_descriptor = CreateFile(filename,
		oflag1,
		oflag2,
	    NULL,
		oflag3,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL);


	if ( INVALID_HANDLE_VALUE == file_descriptor ){
		return NULL;
	}

	HANDLE map_handle          = CreateFileMapping(
	    file_descriptor,
	    NULL,
		oflag4,
	    0,
		block_size,
		mapping_name);

	if ( NULL == map_handle ){
		return NULL;
	}

	mmap_block = (gnb_mmap_block_t *)malloc(sizeof(gnb_mmap_block_t));

	snprintf(mmap_block->filename, PATH_MAX, "%s", filename);

	mmap_block->file_descriptor = file_descriptor;

	mmap_block->map_handle = map_handle;

	mmap_block->block_size = block_size;

	mmap_block->block = MapViewOfFile(
		mmap_block->map_handle,
		prot,
        0,
        0,
		mmap_block->block_size);


	mmap_block->mmap_type = mmap_type;

	return mmap_block;

}

void gnb_mmap_release(gnb_mmap_block_t *mmap_block){

    UnmapViewOfFile(mmap_block->block);

    CloseHandle(mmap_block->map_handle);

    CloseHandle(mmap_block->file_descriptor);

	free(mmap_block);

}


#endif



void* gnb_mmap_get_block(gnb_mmap_block_t *mmap_block){

	return mmap_block->block;

}

size_t gnb_mmap_get_size(gnb_mmap_block_t *mmap_block){

	return mmap_block->block_size;

}


