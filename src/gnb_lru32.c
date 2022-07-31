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

#include <stdlib.h>
#include <string.h>

#include "gnb_lru32.h"
#include "gnb_doubly_linked_list.h"

static gnb_lru32_node_t *gnb_lru32_node_fixed_pool_pop(gnb_lru32_t *lru){

    gnb_lru32_node_t *lru_node = NULL;

    lru_node = gnb_fixed_pool_pop(lru->lru_node_fixed_pool);

    lru_node->dl_node = gnb_fixed_pool_pop(lru->dl_node_fixed_pool);

    if (0 != lru->block_size) {
        lru_node->udata = gnb_fixed_pool_pop(lru->udata_fixed_pool);
    }

    return lru_node;

}


static void gnb_lru32_node_fixed_pool_push(gnb_lru32_t *lru, gnb_lru32_node_t *lru_node){

    gnb_fixed_pool_push(lru->dl_node_fixed_pool,lru_node->dl_node);

    if (0 != lru->block_size) {
        gnb_fixed_pool_push(lru->udata_fixed_pool,lru_node->udata);
    }

    gnb_fixed_pool_push(lru->lru_node_fixed_pool,lru_node);

}


gnb_lru32_t *gnb_lru32_create(gnb_heap_t *heap, uint32_t max_size,uint32_t block_size){
    
    gnb_lru32_t *lru = (gnb_lru32_t *)gnb_heap_alloc(heap,sizeof(gnb_lru32_t));

    lru->heap = heap;
    
    lru->max_size = max_size;
    
    lru->size = 0;

    lru->block_size = block_size;

    lru->lru_node_map = gnb_hash32_create(heap,max_size,max_size);

    if (NULL==lru->lru_node_map) {
        gnb_heap_free(heap,lru);
        return NULL;
    }

    lru->doubly_linked_list = gnb_doubly_linked_list_create(lru->heap);

    if ( 0 != lru->block_size ) {
        lru->udata_fixed_pool = gnb_fixed_pool_create(heap,max_size, lru->block_size);
    }

    lru->dl_node_fixed_pool  = gnb_fixed_pool_create(heap,max_size, sizeof(gnb_doubly_linked_list_node_t));

    lru->lru_node_fixed_pool = gnb_fixed_pool_create(heap,max_size, sizeof(gnb_lru32_node_t));
    
    return lru;
    
}


void gnb_lru32_release(gnb_lru32_t *lru){

    if ( 0 != lru->block_size ) {
        gnb_fixed_pool_release(lru->heap,lru->udata_fixed_pool);
    }

    gnb_fixed_pool_release(lru->heap,lru->dl_node_fixed_pool);
    gnb_fixed_pool_release(lru->heap,lru->lru_node_fixed_pool);

    gnb_hash32_release(lru->lru_node_map);

    gnb_doubly_linked_list_release(lru->doubly_linked_list);

    gnb_heap_free(lru->heap,lru);
    
}


void* gnb_lru32_put(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data){

    gnb_lru32_node_t *lru_node;

    lru_node = gnb_lru32_get(lru, key, key_len);

    if ( NULL != lru_node ) {

        if (data==lru_node->udata) {
            return NULL;
        }

        lru_node->udata = data;
        return NULL;

    }

    void *pop_udata = NULL;
    if ( lru->size >= lru->max_size ) {
        pop_udata = gnb_lru32_pop_tail(lru);
    }

    lru_node = gnb_lru32_node_fixed_pool_pop(lru);
    
    if (NULL==lru_node) {
        //如果链表满了无法set进去，就把传入的指针返回，让调用者处理
        return data;
    }

    lru_node->udata = data;
    
    gnb_doubly_linked_list_add(lru->doubly_linked_list, lru_node->dl_node);

    gnb_hash32_set(lru->lru_node_map, key, (uint32_t)key_len, lru_node, 0);
    lru_node->kv = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);
    
    gnb_doubly_linked_list_node_set(lru_node->dl_node,lru_node);

    lru->size++;
    
    return pop_udata;

}


void gnb_lru32_store(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data, uint32_t size){

    gnb_lru32_node_t *lru_node;

    lru_node = gnb_lru32_get(lru, key, key_len);

    if ( NULL != lru_node ) {

        if (NULL!=lru_node->udata) {
            gnb_heap_free(lru->heap,lru_node->udata);
        }

        lru_node->udata = gnb_heap_alloc(lru->heap,size);
        memcpy(lru_node->udata,data,size);
        
        return;
    }

    void *pop_udata = NULL;

    if ( lru->size >= lru->max_size ) {
        pop_udata = gnb_lru32_pop_tail(lru);
        if (NULL!=pop_udata) {
            gnb_heap_free(lru->heap,pop_udata);
        }
    }

    lru_node = gnb_lru32_node_fixed_pool_pop(lru);
    
    if (NULL==lru_node) {
        return;
    }

    lru_node->udata = gnb_heap_alloc(lru->heap,size);
    memcpy(lru_node->udata,data,size);
    
    gnb_doubly_linked_list_add(lru->doubly_linked_list, lru_node->dl_node);

    gnb_hash32_set(lru->lru_node_map, key, (uint32_t)key_len, lru_node, 0);
    lru_node->kv = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);
    
    gnb_doubly_linked_list_node_set(lru_node->dl_node,lru_node);
    
    lru->size++;

    return;

}


void gnb_lru32_fixed_store(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len, void *data){

    gnb_lru32_node_t *lru_node;

    if ( 0 == lru->block_size ) {
        return;
    }

    lru_node = gnb_lru32_get(lru, key, key_len);

    if ( NULL != lru_node ) {
        memcpy(lru_node->udata,data,lru->block_size);
        return;
    }
    
    void *pop_udata = NULL;

    if ( lru->size >= lru->max_size ) {
        pop_udata = gnb_lru32_pop_tail(lru);
    }
    
    lru_node = gnb_lru32_node_fixed_pool_pop(lru);
    
    if ( NULL == lru_node ) {
        return;
    }

    memcpy(lru_node->udata,data,lru->block_size);

    gnb_doubly_linked_list_add(lru->doubly_linked_list, lru_node->dl_node);

    gnb_hash32_set(lru->lru_node_map, key, (uint32_t)key_len, lru_node, 0);
    lru_node->kv = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);
    
    gnb_doubly_linked_list_node_set(lru_node->dl_node,lru_node);
    
    lru->size++;

    return;
    
}


gnb_lru32_node_t* gnb_lru32_hash_get(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len){

    gnb_kv32_t *kv32 = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);
    
    if (NULL==kv32) {
        return NULL;
    }
    gnb_lru32_node_t *lru_node = GNB_HASH32_VALUE_PTR(kv32);

    return lru_node;
}


gnb_lru32_node_t* gnb_lru32_get(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len){

    gnb_kv32_t *kv32 = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);

    if (NULL==kv32) {
        return NULL;
    }
    gnb_lru32_node_t *lru_node = GNB_HASH32_VALUE_PTR(kv32);

    if ( NULL != lru_node ) {
        gnb_doubly_linked_list_move_head(lru->doubly_linked_list, lru_node->dl_node);
    }
    
    return lru_node;
    
}


void gnb_lru32_movetohead(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len){

    gnb_kv32_t *kv32 = gnb_hash32_get(lru->lru_node_map, key, (uint32_t)key_len);

    if (NULL==kv32) {
        return;
    }

    gnb_lru32_node_t *lru_node = GNB_HASH32_VALUE_PTR(kv32);
    
    if ( NULL != lru_node ) {
        gnb_doubly_linked_list_move_head(lru->doubly_linked_list, lru_node->dl_node);
    }

}


void* gnb_lru32_pop_by_key(gnb_lru32_t *lru, unsigned char *key, uint32_t key_len){

    gnb_lru32_node_t *pop_lru_node;

    gnb_doubly_linked_list_node_t *dl_node;

    void *pop_udata;

    pop_lru_node = gnb_lru32_get(lru, key, key_len);

    if ( NULL == pop_lru_node ) {
        return NULL;
    }

    dl_node = pop_lru_node->dl_node;

    gnb_doubly_linked_list_pop(lru->doubly_linked_list, dl_node);

    pop_udata = pop_lru_node->udata;

    gnb_kv32_t *kv32 = gnb_hash32_del(lru->lru_node_map, pop_lru_node->kv->key->data, pop_lru_node->kv->key->size);

    if (NULL!=kv32) {
        gnb_kv32_release(lru->lru_node_map,kv32);
    }

    gnb_lru32_node_fixed_pool_push(lru, pop_lru_node);

    lru->size--;

    return pop_udata;

}


void* gnb_lru32_pop_head(gnb_lru32_t *lru){
    
    void *pop_udata = NULL;

    gnb_lru32_node_t *pop_lru_node;

    gnb_doubly_linked_list_node_t *head_dl_node;

    if ( 0 == lru->size || NULL==lru->doubly_linked_list->head ) {
        return NULL;
    }

    pop_lru_node = (gnb_lru32_node_t *)lru->doubly_linked_list->head->data;
        
    if ( NULL==pop_lru_node ) {
        return NULL;
    }

    pop_udata = pop_lru_node->udata;

    gnb_kv32_t *kv32 = gnb_hash32_del(lru->lru_node_map, pop_lru_node->kv->key->data, pop_lru_node->kv->key->size);

    if (NULL!=kv32) {
        gnb_kv32_release(lru->lru_node_map,kv32);
    }

    gnb_lru32_node_fixed_pool_push(lru, pop_lru_node);

    head_dl_node = gnb_doubly_linked_list_pop_head(lru->doubly_linked_list);
    
    lru->size--;
    
    return pop_udata;
    
}


void* gnb_lru32_pop_tail(gnb_lru32_t *lru){

    void *pop_udata = NULL;

    gnb_lru32_node_t *pop_lru_node;

    if ( 0 == lru->size || NULL==lru->doubly_linked_list->tail ) {
        return NULL;
    }

    gnb_doubly_linked_list_node_t *tail_dl_node;

    pop_lru_node = (gnb_lru32_node_t *)lru->doubly_linked_list->tail->data;

    if (NULL==pop_lru_node) {
        return NULL;
    }

    pop_udata = pop_lru_node->udata;

    gnb_kv32_t *kv32 = gnb_hash32_del(lru->lru_node_map, pop_lru_node->kv->key->data, pop_lru_node->kv->key->size);

    if (NULL!=kv32) {
        gnb_kv32_release(lru->lru_node_map,kv32);
    }

    gnb_lru32_node_fixed_pool_push(lru, pop_lru_node);

    tail_dl_node = gnb_doubly_linked_list_pop_tail(lru->doubly_linked_list);

    lru->size--;

    return pop_udata;

}


void* gnb_lru32_get_head(gnb_lru32_t *lru){

    void *pop_udata = NULL;

    gnb_lru32_node_t *pop_lru_node;

    if ( 0 == lru->size || NULL==lru->doubly_linked_list->head ) {
        return NULL;
    }

    pop_lru_node = (gnb_lru32_node_t *)lru->doubly_linked_list->head->data;

    if ( NULL==pop_lru_node ) {
        return NULL;
    }

    pop_udata = pop_lru_node->udata;

    return pop_udata;

}


void* gnb_lru32_get_tail(gnb_lru32_t *lru){

    void *pop_udata = NULL;

    gnb_lru32_node_t *pop_lru_node;

    if ( 0 == lru->size || NULL==lru->doubly_linked_list->tail ) {
        return NULL;
    }

    pop_lru_node = (gnb_lru32_node_t *)lru->doubly_linked_list->tail->data;

    if (NULL==pop_lru_node) {
        return NULL;
    }

    pop_udata = pop_lru_node->udata;

    return pop_udata;

}
