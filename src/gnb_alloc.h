#ifndef gnb_alloc_h
#define gnb_alloc_h

#include <stdio.h>
#include <stdint.h>

typedef struct _gnb_heap_fragment_t gnb_heap_fragment_t;

typedef struct _gnb_heap_t{

    uint32_t max_fragment;
    uint32_t fragment_nums;
    uint32_t alloc_byte;
    uint32_t ralloc_byte;
    gnb_heap_fragment_t *fragment_list[0];

}gnb_heap_t;

gnb_heap_t* gnb_heap_create(uint32_t max_fragment);

void* gnb_heap_alloc(gnb_heap_t *gnb_heap, uint32_t size);

void gnb_heap_free(gnb_heap_t *gnb_heap, void *p);

void gnb_heap_clean(gnb_heap_t *gnb_heap);

void gnb_heap_release(gnb_heap_t *gnb_heap);

#endif
