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

#include "stdlib.h"
#include "string.h"

#include "gnb_alloc.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of

#define container_of(ptr, type, member) ({                                    \
                      const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
                       (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


typedef struct _gnb_heap_fragment_t{
    uint32_t size;
    uint32_t idx;
    unsigned char block[0];
}gnb_heap_fragment_t;


gnb_heap_t* gnb_heap_create(uint32_t max_fragment){
    
    gnb_heap_t *gnb_heap = (gnb_heap_t *)malloc( sizeof(gnb_heap_t) + sizeof(gnb_heap_fragment_t) * max_fragment );

    if (NULL==gnb_heap) {
        return NULL;
    }

    memset(gnb_heap, 0, sizeof(gnb_heap_t));

    gnb_heap->max_fragment = max_fragment;
    gnb_heap->fragment_nums  = 0;

    return gnb_heap;
    
}


void* gnb_heap_alloc(gnb_heap_t *gnb_heap, uint32_t size){

    if ( 0 == size ) {
        return NULL;
    }

    if ( gnb_heap->max_fragment == gnb_heap->fragment_nums ) {
        printf("gnb heap is full\n");
        exit(1);
        return NULL;
    }

    if ( size > (uint32_t)(1024l * 1024l * 1024l) - 1 ) {
        return NULL;
    }

    gnb_heap_fragment_t *fragment = malloc( sizeof(gnb_heap_fragment_t) + sizeof(unsigned char) * size );

    if ( NULL == fragment ) {
        return NULL;
    }

    fragment->idx = gnb_heap->fragment_nums;
    gnb_heap->fragment_list[ gnb_heap->fragment_nums ] = fragment;
    gnb_heap->fragment_nums++;

    gnb_heap->alloc_byte  += size;
    gnb_heap->ralloc_byte += sizeof(gnb_heap_fragment_t)+size;

    return (void *)fragment->block;

}

#if 1
void gnb_heap_free(gnb_heap_t *gnb_heap, void *p){

    gnb_heap_fragment_t *fragment;
    
    if ( 0 == gnb_heap->fragment_nums ) {
        //发生错误了
        return;
    }

    if ( NULL == p ) {
        return;
    }
    
    fragment = container_of(p, struct _gnb_heap_fragment_t, block);

    gnb_heap_fragment_t *last_fragment = gnb_heap->fragment_list[gnb_heap->fragment_nums - 1];

    if ( fragment->idx > (gnb_heap->max_fragment-1)  ) {
        //发生错误了
        return;
    }
    
    if ( last_fragment->idx > (gnb_heap->max_fragment-1) ) {
        //发生错误了
        return;
    }
    
    if ( 1 == gnb_heap->fragment_nums ) {

        if ( fragment->idx != last_fragment->idx ) {
            //发生错误了
            return;
        }
        goto finish;

    }

    if ( last_fragment->idx == fragment->idx ) {
        goto finish;
    }

    gnb_heap->alloc_byte -= fragment->size;
    gnb_heap->ralloc_byte -= (sizeof(gnb_heap_fragment_t)+fragment->size);

    last_fragment->idx = fragment->idx;

    gnb_heap->fragment_list[last_fragment->idx] = last_fragment;

finish:

    gnb_heap->fragment_nums--;

    free(fragment);

}
#endif

void gnb_heap_clean(gnb_heap_t *gnb_heap){
    
    int i;
    
    if ( 0 == gnb_heap->fragment_nums ) {
        return;
    }
    
    for ( i=0; i < gnb_heap->fragment_nums; i++ ) {
        free(gnb_heap->fragment_list[i]);
    }
    
    gnb_heap->fragment_nums = 0;
    gnb_heap->alloc_byte  = 0;
    gnb_heap->ralloc_byte = 0;

}


void gnb_heap_release(gnb_heap_t *gnb_heap){
    
    gnb_heap_clean(gnb_heap);

    free(gnb_heap);
    
}
