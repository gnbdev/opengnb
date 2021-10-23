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

#ifndef GNB_LRU32_H
#define GNB_LRU32_H

#include <stdio.h>
#include <stdint.h>

#include "gnb_alloc.h"
#include "gnb_fixed_pool.h"
#include "gnb_hash32.h"
#include "gnb_doubly_linked_list.h"

typedef struct _gnb_lru32_t gnb_lru32_t;
typedef struct _gnb_lru32_node_t gnb_lru32_node_t;

typedef struct _gnb_lru32_node_t{
    gnb_doubly_linked_list_node_t *dl_node;           // dll_node->data -> gnb_lru32_node_t*
    gnb_kv32_t *kv;                                   // 保存在 hashmap 中的kv，使得在双向链表这端可以通过key去存取 hashmap
    void *udata;                                      // -> lru payload
}gnb_lru32_node_t;


typedef struct _gnb_lru32_t{

    gnb_heap_t *heap;

    gnb_doubly_linked_list_t *doubly_linked_list;
    
    gnb_hash32_map_t *lru_node_map;     //save type: gnb_lru32_node_t
    
    uint32_t size;

    uint32_t max_size;

    //lru 可以存数据的指针也可以申请一块内存把数据拷贝到该内存块
    //存入的数据块的大小，如果为0，用同样的key每次set进去都要释放此前为存储数据块申请的内存，
    //如果不等于0，存入数据的时候就使用之前申请好的内存
    uint32_t block_size;

    gnb_fixed_pool_t *lru_node_fixed_pool;
    gnb_fixed_pool_t *dl_node_fixed_pool;
    gnb_fixed_pool_t *udata_fixed_pool;    
    
}gnb_lru32_t;


/*
 如果用 gnb_lru32_set 和 gnb_lru32_store 保存 数据 到 lru， block_size 设为0
 如果用 gnb_lru32_block_set 保存 数据 到 lru， block_size 设为 block 的大小，这对于保存一些容量相同的数据，可以避免频繁申请/释放内存
*/
gnb_lru32_t *gnb_lru32_create(gnb_heap_t *heap, uint32_t max_size,uint32_t block_size);

//需要释放完 lru 里的lru_node->udata，才能执行这个函数
//释放到操作是 迭代执行 gnb_lru32_pop_head 或 gnb_lru32_pop_tail，直到返回为NULL
//由于lru_node->udata存放的是调用者的私有数据，因此释放时需要由调用者自行处理
void gnb_lru32_release(gnb_lru32_t *lru);


//这个函数传入的 data 需要调用者申请内存和释放内存
//如果set入data前发现链表已经满了，就立即返回当前传入的data由调用者释放
//如果set入data后发现链表满了，就dropt掉tail，并返回tail的data由调用者释放
void* gnb_lru32_put(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data);
#define GNB_LRU32_PUT(lru,key,key_len,data) gnb_lru32_put(lru, (unsigned char *)key, (uint32_t)key_len, data)


//传入的 data 会在 lru 内部申请一块内存，拷贝一份，当链表满了需要丢弃tail的节点时，tail节点绑定的这块内存也会被gnb_lru32_set释放
void gnb_lru32_store(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data, uint32_t size);
#define GNB_LRU32_STORE(lru,key,key_len,data,size) gnb_lru32_store(lru, (unsigned char *)key, (uint32_t)key_len, data, (uint32_t)size)

//传入的data会拷贝到lru中一块内存里，相同的key存入数据会覆盖之前的数据，
//每个key对应的内存块是预先申请好的，大小一致的，在调用gnb_lru32_creates时通过block_size设定
//用这个函数保存小块数据到lru是效率最高的，不需要频繁申请/释放内存
void gnb_lru32_fixed_store(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data);
#define GNB_LRU32_FIXED_STORE(lru,key,key_len,data) gnb_lru32_fixed_store(lru,(unsigned char *)key,(uint32_t)key_len, data)


//这个函数不会把命中的节点移到链表首部，需要调用 gnb_lru32_movetohead 实现
gnb_lru32_node_t* gnb_lru32_hash_get(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len);
#define GNB_LRU32_HASH_GET(lru,key,key_len)  gnb_lru32_hash_get(lru,key,(uint32_t)key_len);

//如果命中，会调用 gnb_lru32_movetohead 把命中的节点移动到链表首部
gnb_lru32_node_t* gnb_lru32_get(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len);
#define GNB_LRU32_GET(lru,key,key_len)  gnb_lru32_get(lru,key,(uint32_t)key_len);


#define GNB_LRU32_VALUE(b) b!=NULL?b->udata:NULL
#define GNB_LRU32_HASH_GET_VALUE(lru,key,key_len) GNB_LRU32_VALUE( gnb_lru32_hash_get(lru, (unsigned char *)key, (uint32_t)key_len) )
#define GNB_LRU32_GET_VALUE(lru,key,key_len) GNB_LRU32_VALUE( gnb_lru32_get(lru, (unsigned char *)key, (uint32_t)key_len) )


void gnb_lru32_movetohead(gnb_lru32_t *lru,  unsigned char *key, uint32_t key_len);
#define GNB_LRU32_MOVETOHEAD(lru,key,key_len)  gnb_lru32_movetohead(lru, (unsigned char *)key, (uint32_t)key_len)

void* gnb_lru32_pop_by_key(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len);
#define GNB_LRU32_POP_BY_KEY(lru,key,key_len)  gnb_lru32_pop_by_key(lru,key,(uint32_t)key_len)


void* gnb_lru32_pop_head(gnb_lru32_t *lru);
void* gnb_lru32_pop_tail(gnb_lru32_t *lru);

void* gnb_lru32_get_head(gnb_lru32_t *lru);
void* gnb_lru32_get_tail(gnb_lru32_t *lru);

#define GNB_LRU32_UINT32_PUT(lru,key,data)         gnb_lru32_put(lru, (unsigned char *)(&key), sizeof(uint32_t), data)
#define GNB_LRU32_UINT32_STORE(lru,key,data,size)  gnb_lru32_store(lru, (unsigned char *)(&key), sizeof(uint32_t), data, (uint32_t)size)
#define GNB_LRU32_UINT32_FIX_STORE(lru,key,data)   gnb_lru32_fix_store(lru, (unsigned char *)(&key), sizeof(uint32_t), data)
#define GNB_LRU32_UINT32_HASH_GET(lru,key)         gnb_lru32_hash_get(lru,(unsigned char *)*&key),sizeof(uint32_t));
#define GNB_LRU32_UINT32_GET(lru,key)              gnb_lru32_get(lru,(unsigned char *)(&key),sizeof(uint32_t));

#define GNB_LRU32_UINT32_POP_BY_KEY(lru,key)       gnb_lru32_pop_by_key(lru, (unsigned char *)(&key), sizeof(uint32_t))
#define GNB_LRU32_UINT32_MOVETOHEAD(lru,key)       gnb_lru32_movetohead(lru, (unsigned char *)(&key), sizeof(uint32_t))

#define GNB_LRU32_UINT32_HASH_GET_VALUE(lru,key)  GNB_LRU32_VALUE( gnb_lru32_hash_get(lru, (unsigned char *)(&key), sizeof(uint32_t)) )
#define GNB_LRU32_UINT32_GET_VALUE(lru,key)       GNB_LRU32_VALUE( gnb_lru32_get(lru, (unsigned char *)(&key), sizeof(uint32_t)) )


#endif
