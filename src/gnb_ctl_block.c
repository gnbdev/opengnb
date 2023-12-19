/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "gnb_ctl_block.h"
#include "gnb_mmap.h"
#include "gnb_time.h"
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


gnb_ctl_block_t *gnb_ctl_block_build(void *memory, uint32_t payload_block_size, size_t node_num, uint8_t pf_worker_num){

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
    snprintf((char *)ctl_block->magic_number->data, 16, "%s", "GNB Ver1.4.5.a");

    ctl_block->entry_table256[GNB_CTL_CONF] = off_set;
    block = memory + ctl_block->entry_table256[GNB_CTL_CONF];
    block->size = sizeof(gnb_ctl_conf_zone_t);
    ctl_block->conf_zone = (gnb_ctl_conf_zone_t *)block->data;
    off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_conf_zone_t);
    snprintf((char *)ctl_block->conf_zone->name,    8, "%s", "CONF");

    ctl_block->entry_table256[GNB_CTL_CORE] = off_set;
    block = memory + ctl_block->entry_table256[GNB_CTL_CORE];
    block->size = sizeof(gnb_ctl_core_zone_t) + ( sizeof(gnb_payload16_t) + payload_block_size + sizeof(gnb_payload16_t) + payload_block_size ) * (1+pf_worker_num);
    ctl_block->core_zone = (gnb_ctl_core_zone_t *)block->data;
    off_set += sizeof(gnb_block32_t) + sizeof(gnb_ctl_core_zone_t) + ( sizeof(gnb_payload16_t) + payload_block_size + sizeof(gnb_payload16_t) + payload_block_size ) * (1+pf_worker_num);

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

    gnb_ctl_block_t ctl_block_st;

    gnb_ctl_block_setup(&ctl_block_st, memory);

    ctl_block_st.status_zone->keep_alive_ts_sec = gnb_timestamp_sec();

    //写入 GNB 确定共享内存初始化完成
    char *mem_magic_number = memory;
    mem_magic_number[0] = 'G';
    mem_magic_number[1] = 'N';
    mem_magic_number[2] = 'B';

}


void gnb_ctl_block_setup(gnb_ctl_block_t *ctl_block, void *memory){

    gnb_block32_t *block;

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

}

/*
 flag = 0 readonly
*/
gnb_ctl_block_t *gnb_get_ctl_block(const char *ctl_block_file, int flag){

    ssize_t ctl_file_size = 0;

    uint64_t now_sec;

    gnb_mmap_block_t *mmap_block;

    void *memory;

    char *mem_magic_number;

    gnb_ctl_block_t *ctl_block;

    ctl_file_size = gnb_ctl_file_size(ctl_block_file);

    if ( ctl_file_size < (ssize_t)MIN_CTL_BLOCK_FILE_SIZE ) {
        return NULL;
    }

    mmap_block = gnb_mmap_create(ctl_block_file, ctl_file_size, GNB_MMAP_TYPE_READWRITE);

    if ( NULL == mmap_block ) {
        return NULL;
    }

    memory = gnb_mmap_get_block(mmap_block);

    mem_magic_number = (char *)memory;

    if ( 'G' != mem_magic_number[0] || 'N' != mem_magic_number[1] || 'B' != mem_magic_number[2] ) {
        return NULL;
    }

    now_sec = gnb_timestamp_sec();

    ctl_block = (gnb_ctl_block_t *)malloc(sizeof(gnb_ctl_block_t));

    gnb_ctl_block_setup(ctl_block, memory);

    ctl_block->mmap_block = mmap_block;

    if ( 0 == flag ) {
        return ctl_block;
    }

    if ( now_sec < ctl_block->status_zone->keep_alive_ts_sec ) {
        goto finish_error;
    }

    if ( now_sec == ctl_block->status_zone->keep_alive_ts_sec ) {
        return ctl_block;
    }

    if ( (now_sec - ctl_block->status_zone->keep_alive_ts_sec) < GNB_CTL_KEEP_ALIVE_TS ) {
        return ctl_block;
    }

finish_error:

    gnb_mmap_release(mmap_block);

    free(ctl_block);

    return NULL;

}
