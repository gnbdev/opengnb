#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <sys/stat.h>

#include "gnb_ctl_block.h"

#include "gnb_mmap.h"


#include "gnb_block.h"


//entry_table256 的类型是 uint32_t,
//索引从2 开始，留出 0,1 两个 uint32_t 共 8个字节
//用于 file magic number 和 记录文件的大小

#define GNB_CTL_MAGIC_NUMBER  2
#define GNB_CTL_CONF          3
#define GNB_CTL_CORE          4
#define GNB_CTL_STATUS        5
#define GNB_CTL_NODE          6


ssize_t gnb_ctl_file_size(const char *filename) {

	struct stat s;

    int ret;

    ret = stat(filename,&s);

    if ( 0 != ret ) {
        return -1;
    }

    return s.st_size;

}


gnb_ctl_block_t *gnb_ctl_block_build(void *memory, size_t node_num){

	uint32_t off_set = sizeof(uint32_t)*256;

	gnb_block32_t *block;

	gnb_ctl_block_t *ctl_block = (gnb_ctl_block_t *)malloc(sizeof(gnb_ctl_block_t));

	ctl_block->entry_table256 = memory;

	//向量表清零
	memset(ctl_block->entry_table256, 0, sizeof(uint32_t)*256);

	ctl_block->entry_table256[GNB_CTL_MAGIC_NUMBER] = off_set;

	block = memory + ctl_block->entry_table256[GNB_CTL_MAGIC_NUMBER];
	block->size = sizeof(gnb_ctl_magic_number_t);
	ctl_block->magic_number = (gnb_ctl_magic_number_t *)block->data;
	off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_magic_number_t);
	snprintf((char *)ctl_block->magic_number->data, 16, "%s", "GNB Ver1.1");

	ctl_block->entry_table256[GNB_CTL_CONF] = off_set;
	block = memory + ctl_block->entry_table256[GNB_CTL_CONF];
	block->size = sizeof(gnb_ctl_conf_zone_t);
	ctl_block->conf_zone = (gnb_ctl_conf_zone_t *)block->data;
	off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_conf_zone_t);
	snprintf((char *)ctl_block->conf_zone->name,    8, "%s", "CONF");


	ctl_block->entry_table256[GNB_CTL_CORE] = off_set;
	block = memory + ctl_block->entry_table256[GNB_CTL_CORE];
	block->size = sizeof(gnb_ctl_core_zone_t);
	ctl_block->core_zone = (gnb_ctl_core_zone_t *)block->data;
	off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_core_zone_t);


	ctl_block->entry_table256[GNB_CTL_STATUS] = off_set;
	block = memory + ctl_block->entry_table256[GNB_CTL_STATUS];
	block->size = sizeof(gnb_ctl_status_zone_t);
	ctl_block->status_zone = (gnb_ctl_status_zone_t *)block->data;
	off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_status_zone_t);


	ctl_block->entry_table256[GNB_CTL_NODE] = off_set;

	block = memory + ctl_block->entry_table256[GNB_CTL_NODE];

	block->size = sizeof(gnb_ctl_node_zone_t) + sizeof(gnb_node_t)*node_num;

	ctl_block->node_zone = (gnb_ctl_node_zone_t *)block->data;

	off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_node_zone_t) + sizeof(gnb_node_t)*node_num;

	snprintf((char *)ctl_block->node_zone->name,    8, "%s", "NODE");
	ctl_block->node_zone->node_num = node_num;

	return ctl_block;

}


void gnb_ctl_block_build_finish(void *memory){

	//最后写入 mem magic number 确定共享内存初始化完成
	char *mem_magic_number = memory;
	mem_magic_number[0] = 'G';
	mem_magic_number[1] = 'N';
	mem_magic_number[2] = 'B';

}


gnb_ctl_block_t *gnb_ctl_block_setup(void *memory){

	gnb_block32_t *block;

	gnb_ctl_block_t *ctl_block = (gnb_ctl_block_t *)malloc(sizeof(gnb_ctl_block_t));

	ctl_block->entry_table256 = memory;

	block = memory + ctl_block->entry_table256[GNB_CTL_MAGIC_NUMBER];
	ctl_block->magic_number = (gnb_ctl_magic_number_t *)block->data;

	block = memory + ctl_block->entry_table256[GNB_CTL_CONF];
	ctl_block->conf_zone = (gnb_ctl_conf_zone_t *)block->data;

	block = memory + ctl_block->entry_table256[GNB_CTL_CORE];
	ctl_block->core_zone = (gnb_ctl_core_zone_t *)block->data;

	block = memory + ctl_block->entry_table256[GNB_CTL_STATUS];
	ctl_block->status_zone = (gnb_ctl_status_zone_t *)block->data;

	block = memory + ctl_block->entry_table256[GNB_CTL_NODE];
	ctl_block->node_zone = (gnb_ctl_node_zone_t *)block->data;

	return ctl_block;

}



gnb_ctl_block_t *gnb_ctl_block_init_readwrite(const char *ctl_block_file){

	gnb_ctl_block_t *ctl_block;

	gnb_mmap_block_t *mmap_block;

	void *memory;

	ssize_t file_size = gnb_ctl_file_size(ctl_block_file);

	if( file_size<=0 ){
		return NULL;
	}

	mmap_block = gnb_mmap_create(ctl_block_file, file_size, GNB_MMAP_TYPE_READWRITE);

	if (NULL==mmap_block){
		return NULL;
	}

	memory = gnb_mmap_get_block(mmap_block);

	if ( NULL == memory ){
		return NULL;
	}

	ctl_block = gnb_ctl_block_setup(memory);

	return ctl_block;

}


gnb_ctl_block_t *gnb_ctl_block_init_readonly(const char *ctl_block_file){

	gnb_ctl_block_t *ctl_block;

	gnb_mmap_block_t *mmap_block;

	void *memory;

	ssize_t file_size = gnb_ctl_file_size(ctl_block_file);

	if( file_size<=0 ){
		return NULL;
	}

	mmap_block = gnb_mmap_create(ctl_block_file, file_size, GNB_MMAP_TYPE_READONLY);

	if (NULL==mmap_block){
		return NULL;
	}

	memory = gnb_mmap_get_block(mmap_block);

	if ( NULL == memory ){
		return NULL;
	}

	ctl_block = gnb_ctl_block_setup(memory);

	return ctl_block;
}

