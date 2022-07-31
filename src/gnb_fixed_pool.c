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

#include "gnb_fixed_pool.h"

typedef struct _gnb_fixed_pool_t{

    uint32_t num;

    uint32_t bsize;

    void *block;

    uint32_t array_len;

    void *array[0];

}gnb_fixed_pool_t;


gnb_fixed_pool_t* gnb_fixed_pool_create(gnb_heap_t *heap, uint32_t array_len, uint32_t bsize){

    gnb_fixed_pool_t *fixed_pool;

    fixed_pool = (gnb_fixed_pool_t *)gnb_heap_alloc(heap, sizeof(gnb_fixed_pool_t) + sizeof(void *) * array_len );

    fixed_pool->num = array_len;
    fixed_pool->bsize = bsize;
    fixed_pool->block = gnb_heap_alloc(heap,bsize * array_len);

    memset(fixed_pool->block, 0, bsize * array_len);

    fixed_pool->array_len = array_len;

    int i;

    void *p;

    p = fixed_pool->block;
    
    for (i=0; i<array_len; i++) {
        fixed_pool->array[i] = p;
        p += bsize;
    }

    return fixed_pool;

}


void* gnb_fixed_pool_pop(gnb_fixed_pool_t *fixed_pool){

    if ( 0 == fixed_pool->num ) {
        return NULL;
    }

    void *block;

    block = fixed_pool->array[fixed_pool->num-1];
    fixed_pool->array[fixed_pool->num-1] = NULL;
    fixed_pool->num--;

    return block;

}


uint32_t gnb_fixed_pool_push(gnb_fixed_pool_t *fixed_pool, void *block){

    if ( fixed_pool->array_len == fixed_pool->num ) {
        return 0;
    }

    if ( NULL != fixed_pool->array[fixed_pool->num] ) {
        return -1;
    }

    fixed_pool->array[fixed_pool->num] = block;
    fixed_pool->num++;

    return fixed_pool->num;

}

void gnb_fixed_pool_release(gnb_heap_t *heap,gnb_fixed_pool_t *fixed_pool){
    gnb_heap_free(heap, fixed_pool->block);
    gnb_heap_free(heap, fixed_pool);
}
